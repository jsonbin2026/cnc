cmake_minimum_required(VERSION 3.16)

project(gk_engine
    VERSION 0.1.0
    DESCRIPTION "GK engine - CNC teaching simulator core"
    LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

add_compile_options(-Wall -Wextra -Wpedantic -Werror)

option(GK_BUILD_TESTS "Build unit tests" ON)

add_library(gk STATIC
    src/core/gk_error.c
    src/core/gk_log.c
    src/core/gk_mem.c
    src/core/gk_vec.c
    src/core/gk_string.c
    src/core/gk_registry.c
    src/core/gk_math.c
    src/catalog/gk_catalog_data.c
    src/catalog/gk_catalog.c
    src/parser/gk_lexer.c
    src/parser/gk_codes.c
    src/parser/gk_parser.c
    src/motion/gk_state.c
    src/motion/gk_motion.c
    src/motion/gk_move.c
    src/motion/gk_plan.c
    src/motion/gk_voxel.c
    src/motion/gk_transform.c
    src/motion/gk_macro.c
    src/motion/gk_material.c
    src/motion/gk_collision.c
    src/motion/gk_machine.c
    src/motion/gk_action.c
    src/motion/gk_measure.c
    src/motion/gk_teach.c
    src/motion/gk_alarm.c
    src/motion/gk_ui.c
    src/motion/gk_editor.c
    src/motion/gk_file.c
    src/motion/gk_viz.c
    src/motion/gk_iot.c
    src/motion/gk_ai.c
    src/motion/gk_collab.c
    src/motion/gk_physics.c
    src/motion/gk_process.c
    src/motion/gk_cost.c
    src/motion/gk_maint.c
    src/motion/gk_safety.c
    src/motion/gk_real.c
    src/motion/gk_xr.c
    src/motion/gk_ux.c
    src/motion/gk_saga.c
    src/motion/gk_mscale.c
    src/motion/gk_phys.c
    src/motion/gk_cadcam.c
    src/motion/gk_devops.c
    src/motion/gk_a11y.c
    src/motion/gk_learn.c
    src/motion/gk_sysint.c
    src/motion/gk_prod.c
    src/motion/gk_app.c
    src/motion/gk_hw.c
    src/motion/gk_cutd.c
    src/motion/gk_svo.c
    src/motion/gk_csys.c
    src/motion/gk_hmi.c
    src/motion/gk_envd.c
    src/motion/gk_wf.c
    src/motion/gk_scen.c
    src/motion/gk_fscen.c
    src/motion/gk_rec.c
    src/motion/gk_net2.c
    src/motion/gk_toolsys.c
    src/motion/gk_fixture.c
    src/motion/gk_part.c
    src/motion/gk_staff.c
    src/motion/gk_pmgmt.c
    src/motion/gk_qsys.c
    src/motion/gk_procd.c
    src/motion/gk_progflow.c
    src/motion/gk_mspec.c
    src/exec/gk_executor.c
    src/exec/gk_canned.c
    src/exec/gk_tool.c
)

target_include_directories(gk PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

set_target_properties(gk PROPERTIES POSITION_INDEPENDENT_CODE ON)

find_library(GK_MATH_LIB m)
if(GK_MATH_LIB)
    target_link_libraries(gk PUBLIC ${GK_MATH_LIB})
endif()

if(GK_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
