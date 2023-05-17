load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")
load("@com_justbuchanan_rules_qt//:qt.bzl", "qt_cc_library", "qt_ui_library")

cc_library(
    name = "min_max_pyramid",
    srcs = ["min_max_pyramid.cc"],
    hdrs = ["min_max_pyramid.h"],
    deps = [
        "@opencv4//:opencv",
    ],
)

cc_test(
    name = "min_max_pyramid_test",
    srcs = ["min_max_pyramid_test.cc"],
    deps = [
        ":min_max_pyramid",
        "@gtest",
        "@gtest//:gtest_main",
    ],
)

qt_cc_library(
    name = "editor",
    srcs = ["editor.cc"],
    hdrs = ["editor.h"],
    deps = [
        ":min_max_pyramid",
        "@opencv4//:opencv",
        "@qt//:qt_core",
        "@qt//:qt_gui",
        "@qt//:qt_widgets",
    ],
)

cc_binary(
    name = "main",
    srcs = ["main.cc"],
    copts = ["-fPIC"],
    deps = [
        ":editor",
        ":min_max_pyramid",
        "@opencv4//:opencv",
        "@qt//:qt_widgets",
    ],
)
