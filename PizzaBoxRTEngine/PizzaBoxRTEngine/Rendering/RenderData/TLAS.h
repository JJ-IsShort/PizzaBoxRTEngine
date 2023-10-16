#pragma once
#include "AccelerationStructure.h"
#include <vector>

namespace PBEngine
{
    class TLAS {
        public:
            TLAS();
            ~TLAS();

            void AddBLAS(AccelerationStructure* blas) {
                blasList.push_back(std::move(*blas));
            }

            void BuildTLAS();

            VkAccelerationStructureKHR GetHandle()
            {
                return handle;
            }

            //int numInstances{ get {return blasList.Count; } };
            int GetNumInstances() { return blasList.size(); }

            //std::vector<VkAccelerationStructureInstanceKHR> instancesData;
            //std::vector<VkAccelerationStructureGeometryKHR> geometry;

        private:
            std::vector<AccelerationStructure> blasList;

            VkAccelerationStructureKHR handle;
            uint64_t deviceAddress;
            std::unique_ptr<Buffer> buffer;
    };
}