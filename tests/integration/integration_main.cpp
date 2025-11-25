#include <gtest/gtest.h>

#ifdef BEDROCK_WITH_TRANSPORT_DEPS
#include <QCoreApplication>
#include <QDebug>
#endif

int main(int argc, char** argv) {
#ifdef BEDROCK_WITH_TRANSPORT_DEPS
    // Create QCoreApplication for Qt-based tests
    QCoreApplication app(argc, argv);
    
    // Initialize GoogleTest
    ::testing::InitGoogleTest(&argc, argv);
    
    // Run tests
    int result = RUN_ALL_TESTS();
    
    return result;
#else
    // Without transport deps, just run basic GoogleTest
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
#endif
}

