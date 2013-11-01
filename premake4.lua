----------------------------------------------------------------------------------------------------
if _ACTION == "clean" then
    os.rmdir("build")
end

----------------------------------------------------------------------------------------------------
solution "Support"

    configurations { "Debug", "Release" }

    platforms { "x64", "x32" }

    location    ("build/" .. _ACTION)
    objdir      ("build/" .. _ACTION .. "/obj")

    configuration { "Debug" }
        targetsuffix "d"
        defines { "_DEBUG" }
        flags { "ExtraWarnings", "Symbols" }

    configuration { "Release" }
        defines { "NDEBUG" }
        flags { "ExtraWarnings", "Optimize" }

    configuration { "Debug", "x64" }
        targetdir ("build/" .. _ACTION .. "/bin/x64/Debug")

    configuration { "Release", "x64" }
        targetdir ("build/" .. _ACTION .. "/bin/x64/Release")

    configuration { "Debug", "x32" }
        targetdir ("build/" .. _ACTION .. "/bin/x32/Debug")

    configuration { "Release", "x32" }
        targetdir ("build/" .. _ACTION .. "/bin/x32/Release")

    configuration { "gmake" }
        buildoptions {
            "-std=c++11",
            "-pedantic",
--            "-save-temps",
        }

    configuration { "windows" }
        flags { "Unicode" }

----------------------------------------------------------------------------------------------------
local have_gtest = os.isdir("test/gtest")

----------------------------------------------------------------------------------------------------
project "CmdLine"

    kind "StaticLib"

    language "C++"

    includedirs { "include/" }

    files {
        "include/**.*",
        "src/**.*",
    }

----------------------------------------------------------------------------------------------------
project "Test"

    kind "ConsoleApp"

    language "C++"

    links { "CmdLine" }

    includedirs { "include/" }

    files {
        "test/Test.cpp",
        "test/CmdLineQt.h",
    }

----------------------------------------------------------------------------------------------------
project "CmdLineTest"

    kind "ConsoleApp"

    language "C++"

    links { "CmdLine", "gtest", "gtest_main" }

    includedirs { "include/" }

    if have_gtest then
        includedirs { "test/gtest/include" }
    end

    files {
        "test/CmdLineTest.cpp",
    }

----------------------------------------------------------------------------------------------------
project "CmdLineToArgvTest"

    kind "ConsoleApp"

    language "C++"

    links { "CmdLine" }

    includedirs { "include/" }

    files {
        "test/CmdLineToArgvTest.cpp",
    }

----------------------------------------------------------------------------------------------------
project "ConvertUTFTest"

    kind "ConsoleApp"

    language "C++"

    links { "CmdLine", "gtest", "gtest_main" }

    includedirs { "include/" }

    if have_gtest then
        includedirs { "test/gtest/include" }
    end

    files {
        "test/ConvertUTFTest.cpp",
        "test/ConvertUTF.h",
    }

----------------------------------------------------------------------------------------------------
project "StringRefTest"

    kind "ConsoleApp"

    language "C++"

    links { "CmdLine", "gtest", "gtest_main" }

    includedirs { "include/" }

    if have_gtest then
        includedirs { "test/gtest/include" }
    end

    files {
        "test/StringRefTest.cpp",
    }

----------------------------------------------------------------------------------------------------
project "StringSplitTest"

    kind "ConsoleApp"

    language "C++"

    links { "CmdLine", "gtest", "gtest_main" }

    includedirs { "include/" }

    if have_gtest then
        includedirs { "test/gtest/include" }
    end

    files {
        "test/StringSplitTest.cpp",
    }

----------------------------------------------------------------------------------------------------
if have_gtest then

    project "gtest"

        kind "StaticLib"

        language "C++"

        includedirs {
            "test/gtest/include",
            "test/gtest",
        }

        files {
            "test/gtest/src/gtest-all.cc",
        }

    project "gtest_main"

        kind "StaticLib"

        language "C++"

        includedirs {
            "test/gtest/include",
            "test/gtest",
        }

        files {
            "test/gtest/src/gtest_main.cc",
        }

end
