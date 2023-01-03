#include "D3DUtil.h"
#include <comdef.h>
#include <fstream>
#include <d3dcompiler.h>
#include "Utils/Log/Logger.h"

DxException::DxException(HRESULT hr, const CheString& funcName, const CheString& fileName, int lineNo)
    : mErrorCode(hr), mFuncName(funcName), mFileName(fileName), mLineNo(lineNo)
{
}

CheString DxException::ToString() const
{
  _com_error err(mErrorCode);
  CheString msg = err.ErrorMessage();
  return mFuncName + CTEXT("Failed in ") + mFileName + CTEXT("; Line ") + ConvertToCheString(mLineNo) + CTEXT("; Error: ") +
         msg;
}

ComPtr<ID3DBlob> D3DUtil::LoadBinary(const CheString& fileName)
{
  std::ifstream fin(fileName, std::ios::binary);

  fin.seekg(0, std::ios_base::end);
  std::ifstream::pos_type size = (uint32)fin.tellg();
  fin.seekg(0, std::ios_base::beg);

  ComPtr<ID3DBlob> blob;
  TIFF(D3DCreateBlob(size, blob.GetAddressOf()));

  fin.read((char*)blob->GetBufferPointer(), size);
  fin.close();

  return blob;
}

ComPtr<ID3D12Resource> D3DUtil::CreateDefaultBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
                                                    const void* initData, UINT64 byteSize, ComPtr<ID3D12Resource>& uploadBuffer)
{
  ComPtr<ID3D12Resource> defaultBuffer;

  // Create the actual default buffer resource.
  TIFF(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
                                       &CD3DX12_RESOURCE_DESC::Buffer(byteSize), D3D12_RESOURCE_STATE_COMMON, nullptr,
                                       IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

  // In order to copy CPU memory data into our default buffer, we need to create
  // an intermediate upload heap.
  TIFF(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
                                       &CD3DX12_RESOURCE_DESC::Buffer(byteSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                       IID_PPV_ARGS(uploadBuffer.GetAddressOf())));

  // Describe the data we want to copy into the default buffer.
  D3D12_SUBRESOURCE_DATA subResourceData = {};
  subResourceData.pData                  = initData;
  subResourceData.RowPitch               = byteSize;
  subResourceData.SlicePitch             = subResourceData.RowPitch;

  // Schedule to copy the data to the default buffer resource.  At a high level, the helper function UpdateSubresources
  // will copy the CPU memory into the intermediate upload heap.  Then, using ID3D12CommandList::CopySubresourceRegion,
  // the intermediate upload heap data will be copied to mBuffer.
  cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON,
                                                                    D3D12_RESOURCE_STATE_COPY_DEST));
  UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
  cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                                    D3D12_RESOURCE_STATE_GENERIC_READ));

  // Note: uploadBuffer has to be kept alive after the above function calls because
  // the command list has not been executed yet that performs the actual copy.
  // The caller can Release the uploadBuffer after it knows the copy has been executed.

  return defaultBuffer;
}

ComPtr<ID3DBlob> D3DUtil::CompileShader(const CheString& fileName, const D3D_SHADER_MACRO* defines, const CheString& entryPoint,
                                        const CheString& target)
{
  UINT compileFlags = 0;

  HRESULT hr = S_OK;

  ComPtr<ID3DBlob> byteCode = nullptr;
  ComPtr<ID3DBlob> errors;
  hr = D3DCompileFromFile(fileName.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, ConvertToMultiByte(entryPoint).c_str(),
                          ConvertToMultiByte(target).c_str(), compileFlags, 0, &byteCode, &errors);

  if (errors != nullptr) logger.Error(ConvertToCheString((char*)errors->GetBufferPointer()));

  TIFF(hr);

  return byteCode;
}