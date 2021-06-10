#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiDriverEntryPoint.h>

#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/HiiImageDecoder.h>

#include "lodepng/lodepng.h"
#include "lodepng_allocator.h"

STATIC EFI_GUID mDecoderNames[1];

STATIC
EFI_STATUS
EFIAPI
GetImageDecoderName(
  IN      EFI_HII_IMAGE_DECODER_PROTOCOL   *This,
  IN OUT  EFI_GUID                         **DecoderName,
  IN OUT  UINT16                           *NumberOfDecoderName
  )
{
  mDecoderNames[0] = gEfiHiiImageDecoderNamePngGuid;

  *NumberOfDecoderName = 1;
  *DecoderName = mDecoderNames;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
GetImageInfo(
  IN      EFI_HII_IMAGE_DECODER_PROTOCOL           *This,
  IN      VOID                                     *Image,
  IN      UINTN                                    SizeOfImage,
  IN OUT  EFI_HII_IMAGE_DECODER_IMAGE_INFO_HEADER  **ImageInfo
  )
{
  unsigned error;
  unsigned width, height;
  LodePNGState state;
  EFI_HII_IMAGE_DECODER_PNG_INFO *pImageInfo;

  lodepng_state_init(&state);

  error = lodepng_inspect(&width, &height, &state, Image, SizeOfImage);
  if (error)
    return EFI_UNSUPPORTED;

  pImageInfo = AllocatePool(sizeof(*pImageInfo));
  if (!pImageInfo)
    return EFI_BAD_BUFFER_SIZE;

  pImageInfo->Header.DecoderName = gEfiHiiImageDecoderNamePngGuid;
  pImageInfo->Header.ImageInfoSize = sizeof(*pImageInfo);
  pImageInfo->Header.ImageWidth = width;
  pImageInfo->Header.ImageHeight = height;

  switch (state.info_png.color.colortype) {
  case LCT_RGB:
    pImageInfo->Header.ColorType = EFI_HII_IMAGE_DECODER_COLOR_TYPE_RGB;
    break;
  case LCT_RGBA:
    pImageInfo->Header.ColorType = EFI_HII_IMAGE_DECODER_COLOR_TYPE_RGBA;
    break;
  default:
    pImageInfo->Header.ColorType = EFI_HII_IMAGE_DECODER_COLOR_TYPE_UNKNOWN;
  }

  pImageInfo->Header.ColorDepthInBits = state.info_png.color.bitdepth;

  switch (state.info_png.color.colortype) {
  case LCT_GREY:
    pImageInfo->Channels = 1;
    break;
  case LCT_RGB:
    pImageInfo->Channels = 3;
    break;
  case LCT_PALETTE:
    pImageInfo->Channels = 4;
    break;
  case LCT_GREY_ALPHA:
    pImageInfo->Channels = 2;
    break;
  case LCT_RGBA:
    pImageInfo->Channels = 4;
    break;
  default:
    pImageInfo->Channels = 0;
  }

  *ImageInfo = &pImageInfo->Header;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
DecodeImage(
  IN      EFI_HII_IMAGE_DECODER_PROTOCOL  *This,
  IN      VOID                            *Image,
  IN      UINTN                           ImageRawDataSize,
  IN OUT  EFI_IMAGE_OUTPUT                **Bitmap,
  IN      BOOLEAN                         Transparent
  )
{
  unsigned error;
  unsigned width, height;
  unsigned char *lodepng_bitmap = NULL, *lodepng_bitmap_ptr;
  EFI_IMAGE_OUTPUT *pBitmap;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *pPixel;

  error = lodepng_decode24(&lodepng_bitmap, &width, &height, Image, ImageRawDataSize);
  if (error)
    return EFI_UNSUPPORTED;

  pBitmap = AllocatePool(sizeof(*pBitmap));
  if (!pBitmap)
    return EFI_BAD_BUFFER_SIZE;

  pBitmap->Image.Bitmap = AllocateZeroPool(width * height *
                                       sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
  if (!pBitmap->Image.Bitmap) {
    FreePool(pBitmap);
    return EFI_BAD_BUFFER_SIZE;
  }

  pBitmap->Width = width;
  pBitmap->Height = height;

  lodepng_bitmap_ptr = lodepng_bitmap;
  pPixel = pBitmap->Image.Bitmap;
  for (UINTN i = 0; i < width * height; i++) {
    pPixel->Red = *(lodepng_bitmap_ptr++);
    pPixel->Green = *(lodepng_bitmap_ptr++);
    pPixel->Blue = *(lodepng_bitmap_ptr++);
    pPixel++;
  }

  lodepng_free(lodepng_bitmap);

  *Bitmap = pBitmap;

  return EFI_SUCCESS;
}

STATIC EFI_HII_IMAGE_DECODER_PROTOCOL gLodePngDecodeProtocol = {
  GetImageDecoderName,
  GetImageInfo,
  DecodeImage,
};

EFI_STATUS
EFIAPI
LodePngDecodeEntry(
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS Status;

  Status = gBS->InstallMultipleProtocolInterfaces(&ImageHandle,
                                                  &gEfiHiiImageDecoderProtocolGuid,
                                                  &gLodePngDecodeProtocol,
                                                  NULL);

  return Status;
}
