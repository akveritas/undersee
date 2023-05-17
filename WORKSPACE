# Configuration to get all the various dependencies working with Bazel. Some
# projects are bazel-native, so git_repository is sufficient; others take a
# little more wrangling.
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "gtest",
    branch = "v1.10.x",
    remote = "https://github.com/google/googletest",
)

new_local_repository(
    name = "opencv4",
    build_file = "opencv.BUILD",
    path = "/usr/include/opencv4/opencv2",
)

# Outsource the wrangling for QT, the GUI library we've chosen.
git_repository(
    name = "com_justbuchanan_rules_qt",
    commit = "3196fcf2e6ee81cf3a2e2b272af3d4259b84fcf9",
    remote = "https://github.com/justbuchanan/bazel_rules_qt.git",
    shallow_since = "1645077947 -0800",
)

load("@com_justbuchanan_rules_qt//:qt_configure.bzl", "qt_configure")

qt_configure()

load("@local_config_qt//:local_qt.bzl", "local_qt_path")

new_local_repository(
    name = "qt",
    build_file = "@com_justbuchanan_rules_qt//:qt.BUILD",
    path = local_qt_path(),
)

load("@com_justbuchanan_rules_qt//tools:qt_toolchain.bzl", "register_qt_toolchains")

register_qt_toolchains()

# Generate compile commands to work with external tooling.
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# Hedron's Compile Commands Extractor for Bazel
# https://github.com/hedronvision/bazel-compile-commands-extractor
# TL;DR refresh the compile commands database by running:
#    $ bazel run @hedron_compile_commands//:refresh_all
http_archive(
    name = "hedron_compile_commands",
    strip_prefix = "bazel-compile-commands-extractor-3dddf205a1f5cde20faf2444c1757abe0564ff4c",

    url = "https://github.com/hedronvision/bazel-compile-commands-extractor/archive/3dddf205a1f5cde20faf2444c1757abe0564ff4c.tar.gz",
    sha256 = "3cd0e49f0f4a6d406c1d74b53b7616f5e24f5fd319eafc1bf8eee6e14124d115"
)

load("@hedron_compile_commands//:workspace_setup.bzl", "hedron_compile_commands_setup")

hedron_compile_commands_setup()
