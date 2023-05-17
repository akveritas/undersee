# Coopt opencv into a bazel package. Also requires a WORKSPACE rule,
# as well as an include directive in .bazelrc
# Note that this relies on opencv4 being installed locally in
#    /usr/include/opencv4
# This is obviously not ideal for 'portable builds' but is the default location
# when installing libopencv-dev from Aptitude.
cc_library(
    name = "opencv",
    hdrs = glob([
        "**/*.h*",
    ]),
    includes = [
        "/usr/include/opencv4",
    ],
    linkopts = [
      "-l:libopencv_core.so",
      "-l:libopencv_calib3d.so",
      "-l:libopencv_features2d.so",
      "-l:libopencv_highgui.so",
      "-l:libopencv_imgcodecs.so",
      "-l:libopencv_imgproc.so",
    ],
    visibility = ["//visibility:public"],
    linkstatic = 1,
)
