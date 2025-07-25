#pragma once

#include <Tempest/AbstractGraphicsApi>
#include <d3d12.h>

#include "gapi/deviceallocator.h"

namespace Tempest {
namespace Detail {

class DxDevice;

class DxDescriptorAllocator {
  public:
    DxDescriptorAllocator();

    struct Provider {
      using DeviceMemory = ID3D12DescriptorHeap*;
      ~Provider();

      DxDevice*    device     = nullptr;
      DeviceMemory last       = nullptr;
      size_t       lastSize   = 0;
      uint32_t     lastTypeId = 0;

      DeviceMemory alloc(size_t size, uint32_t typeId);
      void         free(DeviceMemory m, size_t size, uint32_t typeId);
      };
    void       setDevice(DxDevice& device);

    using Allocation = typename Tempest::Detail::DeviceAllocator<Provider>::Allocation;

    Allocation allocHost(size_t count);
    Allocation allocRtv (size_t count);
    Allocation allocDsv (size_t count);
    void       free (Allocation& page);

    ID3D12DescriptorHeap*       heapof(const Allocation& a);
    D3D12_CPU_DESCRIPTOR_HANDLE handle(const Allocation& a, size_t offset = 0);

  private:
    Provider                          providerRes;
    Detail::DeviceAllocator<Provider> allocatorRes{providerRes};

    uint32_t                          resSize = 1;
    uint32_t                          rtvSize = 1;
    uint32_t                          dsvSize = 1;
  };

}
}


