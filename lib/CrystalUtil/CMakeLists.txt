if (NOT TARGET CrystalUtil)
  message("Fetching CrystalUtil...")
  include(FetchContent)
  FetchContent_Declare(
  CrystalUtil
  GIT_REPOSITORY https://github.com/ray847/CrystalUtil.git
  GIT_TAG "origin/main"
  )
  FetchContent_MakeAvailable(CrystalUtil)
  message("CrystalUtil found.")
endif()
