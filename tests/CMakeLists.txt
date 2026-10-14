# testSupport.hpp, also for projects that build MeshNetwork as a subproject
# and test their own code on top of it.
add_library(MeshNetworkTestSupport INTERFACE testSupport.hpp)
target_include_directories(MeshNetworkTestSupport INTERFACE .)
target_link_libraries(MeshNetworkTestSupport INTERFACE Boost::headers)

add_executable(meshTests meshTests.cpp)
target_link_libraries(meshTests PRIVATE MeshNetwork MeshNetworkTestSupport)

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
    notAcceptingDestinationReportsError
    sendFailsWhenDestinationDisappears
    identicalPayloadsAreEachAcknowledged
    oversizedSendReportsError
    staleAdvertIsWithdrawnAfterReroute
    duplicateLinkSurvivesLosingTheOther
)
foreach(test IN LISTS MESH_TESTS)
    add_test(NAME mesh.${test} COMMAND meshTests ${test})
    set_tests_properties(mesh.${test} PROPERTIES TIMEOUT 30)
endforeach()
