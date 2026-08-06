// Compile-only test for NVMe refactoring headers.
#include <tests/test_framework.h>
#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_interrupt_configurator.h>
#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_queue_setup.h>
#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_register_mapper.h>

static const char* test_nvme_headers_compile() {
  // Spot-check that the refactored NVMe headers define their core types.
  TEST_ASSERT(sizeof(fkernel::NvmeQueueSetup) > 0, "NvmeQueueSetup must be defined");
  TEST_ASSERT(sizeof(fkernel::NvmeRegisterMapper) > 0, "NvmeRegisterMapper must be defined");
  TEST_ASSERT(sizeof(fkernel::NvmeInterruptConfigurator) > 0, "NvmeInterruptConfigurator must be defined");
  return nullptr;
}

static const test_case_t nvme_refactoring_tests[] = {
    {"test_nvme_headers_compile", test_nvme_headers_compile},
};

int run_nvme_refactoring_tests() {
  return run_tests("Driver::Nvme::Refactoring", nvme_refactoring_tests,
                   sizeof(nvme_refactoring_tests) / sizeof(nvme_refactoring_tests[0]));
}
