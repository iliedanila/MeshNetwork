find_package(Boost REQUIRED COMPONENTS serialization)
find_package(Threads REQUIRED)

add_executable(meshTests meshTests.cpp)
target_link_libraries(meshTests MeshNetwork Boost::serialization Threads::Threads)

set(MESH_TESTS
    malformedFrameClosesOnlyThatConnection
    oversizedFrameIsRejected
    nodeConnectsToSeveralPeers
    loggerPrintsItsOwnLogs
    lateJoinerLearnsLogger
    loggerRouteRecoversAfterLoggerRestarts
    noSelfRoutesInTriangle
    triangleWithLoggerGoesQuiet
    departedNodeIsForgottenInRing
)
foreach(test IN LISTS MESH_TESTS)
    add_test(NAME mesh.${test} COMMAND meshTests ${test})
    set_tests_properties(mesh.${test} PROPERTIES TIMEOUT 30)
endforeach()
