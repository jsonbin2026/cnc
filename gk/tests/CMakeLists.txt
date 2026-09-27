add_executable(test_core
    gk_test.c
    test_core.c
)

target_link_libraries(test_core PRIVATE gk)

add_test(NAME core COMMAND test_core)

add_executable(test_parser
    gk_test.c
    test_parser.c
)

target_link_libraries(test_parser PRIVATE gk)

add_test(NAME parser COMMAND test_parser)

add_executable(test_motion
    gk_test.c
    test_motion.c
)

target_link_libraries(test_motion PRIVATE gk)

add_test(NAME motion COMMAND test_motion)

add_executable(test_exec
    gk_test.c
    test_exec.c
)

target_link_libraries(test_exec PRIVATE gk)

add_test(NAME exec COMMAND test_exec)

add_executable(test_tool
    gk_test.c
    test_tool.c
)

target_link_libraries(test_tool PRIVATE gk)

add_test(NAME tool COMMAND test_tool)

add_executable(test_macro
    gk_test.c
    test_macro.c
)

target_link_libraries(test_macro PRIVATE gk)

add_test(NAME macro COMMAND test_macro)

add_executable(test_material
    gk_test.c
    test_material.c
)

target_link_libraries(test_material PRIVATE gk)

add_test(NAME material COMMAND test_material)

add_executable(test_collision
    gk_test.c
    test_collision.c
)

target_link_libraries(test_collision PRIVATE gk)

add_test(NAME collision COMMAND test_collision)

add_executable(test_machine
    gk_test.c
    test_machine.c
)

target_link_libraries(test_machine PRIVATE gk)

add_test(NAME machine COMMAND test_machine)

add_executable(test_action
    gk_test.c
    test_action.c
)

target_link_libraries(test_action PRIVATE gk)

add_test(NAME action COMMAND test_action)

add_executable(test_measure
    gk_test.c
    test_measure.c
)

target_link_libraries(test_measure PRIVATE gk)

add_test(NAME measure COMMAND test_measure)

add_executable(test_teach
    gk_test.c
    test_teach.c
)

target_link_libraries(test_teach PRIVATE gk)

add_test(NAME teach COMMAND test_teach)

add_executable(test_alarm
    gk_test.c
    test_alarm.c
)

target_link_libraries(test_alarm PRIVATE gk)

add_test(NAME alarm COMMAND test_alarm)

add_executable(test_ui
    gk_test.c
    test_ui.c
)

target_link_libraries(test_ui PRIVATE gk)

add_test(NAME ui COMMAND test_ui)

add_executable(test_editor
    gk_test.c
    test_editor.c
)

target_link_libraries(test_editor PRIVATE gk)

add_test(NAME editor COMMAND test_editor)

add_executable(test_ai
    gk_test.c
    test_ai.c
)

target_link_libraries(test_ai PRIVATE gk)

add_test(NAME ai COMMAND test_ai)

add_executable(test_collab
    gk_test.c
    test_collab.c
)

target_link_libraries(test_collab PRIVATE gk)

add_test(NAME collab COMMAND test_collab)

add_executable(test_physics
    gk_test.c
    test_physics.c
)

target_link_libraries(test_physics PRIVATE gk)

add_test(NAME physics COMMAND test_physics)

add_executable(test_process
    gk_test.c
    test_process.c
)

target_link_libraries(test_process PRIVATE gk)

add_test(NAME process COMMAND test_process)

add_executable(test_cost
    gk_test.c
    test_cost.c
)

target_link_libraries(test_cost PRIVATE gk)

add_test(NAME cost COMMAND test_cost)

add_executable(test_maint
    gk_test.c
    test_maint.c
)

target_link_libraries(test_maint PRIVATE gk)

add_test(NAME maint COMMAND test_maint)

add_executable(test_safety
    gk_test.c
    test_safety.c
)

target_link_libraries(test_safety PRIVATE gk)

add_test(NAME safety COMMAND test_safety)

add_executable(test_real
    gk_test.c
    test_real.c
)

target_link_libraries(test_real PRIVATE gk)

add_test(NAME real COMMAND test_real)

add_executable(test_xr
    gk_test.c
    test_xr.c
)

target_link_libraries(test_xr PRIVATE gk)

add_test(NAME xr COMMAND test_xr)

add_executable(test_ux
    gk_test.c
    test_ux.c
)

target_link_libraries(test_ux PRIVATE gk)

add_test(NAME ux COMMAND test_ux)

add_executable(test_saga
    gk_test.c
    test_saga.c
)

target_link_libraries(test_saga PRIVATE gk)

add_test(NAME saga COMMAND test_saga)

add_executable(test_mscale
    gk_test.c
    test_mscale.c
)

target_link_libraries(test_mscale PRIVATE gk)

add_test(NAME mscale COMMAND test_mscale)

add_executable(test_cadcam
    gk_test.c
    test_cadcam.c
)

target_link_libraries(test_cadcam PRIVATE gk)

add_test(NAME cadcam COMMAND test_cadcam)

add_executable(test_devops
    gk_test.c
    test_devops.c
)

target_link_libraries(test_devops PRIVATE gk)

add_test(NAME devops COMMAND test_devops)

add_executable(test_learn
    gk_test.c
    test_learn.c
)

target_link_libraries(test_learn PRIVATE gk)

add_test(NAME learn COMMAND test_learn)

add_executable(test_sysint
    gk_test.c
    test_sysint.c
)

target_link_libraries(test_sysint PRIVATE gk)

add_test(NAME sysint COMMAND test_sysint)

add_executable(test_prod
    gk_test.c
    test_prod.c
)

target_link_libraries(test_prod PRIVATE gk)

add_test(NAME prod COMMAND test_prod)

add_executable(test_app
    gk_test.c
    test_app.c
)

target_link_libraries(test_app PRIVATE gk)

add_test(NAME app COMMAND test_app)

add_executable(test_hw
    gk_test.c
    test_hw.c
)

target_link_libraries(test_hw PRIVATE gk)

add_test(NAME hw COMMAND test_hw)

add_executable(test_cutd
    gk_test.c
    test_cutd.c
)

target_link_libraries(test_cutd PRIVATE gk)

add_test(NAME cutd COMMAND test_cutd)

add_executable(test_svo
    gk_test.c
    test_svo.c
)

target_link_libraries(test_svo PRIVATE gk)

add_test(NAME svo COMMAND test_svo)

add_executable(test_csys
    gk_test.c
    test_csys.c
)

target_link_libraries(test_csys PRIVATE gk)

add_test(NAME csys COMMAND test_csys)

add_executable(test_hmi
    gk_test.c
    test_hmi.c
)

target_link_libraries(test_hmi PRIVATE gk)

add_test(NAME hmi COMMAND test_hmi)

add_executable(test_envd
    gk_test.c
    test_envd.c
)

target_link_libraries(test_envd PRIVATE gk)

add_test(NAME envd COMMAND test_envd)

macro(gk_add_test name)
    add_executable(test_${name}
        gk_test.c
        test_${name}.c
    )
    target_link_libraries(test_${name} PRIVATE gk)
    add_test(NAME ${name} COMMAND test_${name})
endmacro()

gk_add_test(wf)
gk_add_test(scen)
gk_add_test(fscen)
gk_add_test(rec)
gk_add_test(net2)
gk_add_test(toolsys)
gk_add_test(fixture)
gk_add_test(part)
gk_add_test(staff)
gk_add_test(pmgmt)
gk_add_test(qsys)
gk_add_test(procd)
gk_add_test(progflow)
gk_add_test(mspec)
