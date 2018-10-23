-- premake5.lua
-- cmd> premake5 --file=premake5.lua vs2015

-- solution
workspace "BuildAll"
    configurations { "Debug", "Release" }
    location "Intermedia"
    language "C++"
    architecture "x86"
    
	-- Catch requires RTTI and exceptions
	exceptionhandling "On"
	rtti "On"


    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"
        targetsuffix("_d")

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"
    
    filter "system:windows"
        defines { "_WIN32" }

    filter {}

    targetdir("../Bin")  
    objdir("Intermedia/Obj/%{prj.name}/%{cfg.longname}")
    debugdir("../Bin")
	
-----------------------------------------------------   
-- Helper function for set  ------------
function setup_include_link_env()
	defines { " _CRT_SECURE_NO_WARNINGS" }
	
    includedirs {
		"../Src"
	}
	
 --   libdirs {
 --       "../lib"
 --   }
	
 --	filter "configurations:Debug"
 --		links { }
	
 --	filter "configurations:Release"
 --	links { }
		
end

-- project: example access kernel address space
project "Example_AccKernel"
    kind "ConsoleApp"
    setup_include_link_env()
	files {
        "../Src/Examples/AccKernel.cpp"
    }
    
-- project: example write code page
project "Example_WriteCodePage"
    kind "ConsoleApp"
    setup_include_link_env()
	files {
        "../Src/Examples/WriteCodePage.cpp"
    }
	
-- project: example modify data segment  register. 
project "Example_ProtectSegmenet"
    kind "ConsoleApp"
    setup_include_link_env()
	files {
        "../Src/Examples/ProtectSegmenet.cpp"
    }

-- project: example fault exception. 
project "Example_Fault"
    kind "ConsoleApp"
    setup_include_link_env()
	files {
        "../Src/Examples/Fault.cpp"
    }

-- project: example branch break point. 
project "Example_B2BStep"
    kind "ConsoleApp"
    setup_include_link_env()
	files {
		"../Src/Examples/DvrAgent.h",
		"../Src/Examples/DvrAgent.cpp",
        "../Src/Examples/B2BStep.cpp"
    }

-- project: example int3. 
project "Example_HiInt3"
    kind "ConsoleApp"
    setup_include_link_env()
	files {
        "../Src/Examples/HiInt3.cpp"
    }
	
-- project: example DataBP. 
project "Example_DataBP"
    kind "ConsoleApp"
    setup_include_link_env()
	files {
        "../Src/Examples/DataBP.cpp"
    }

	-- project: example Try int 1. 
project "Example_TryInt1"
    kind "ConsoleApp"
    setup_include_link_env()
	files {
        "../Src/Examples/TryInt1.cpp"
    }
	
	-- project: example Virtual Terminal
project "Example_ConsoleVT"
    kind "ConsoleApp"
    setup_include_link_env()
	files {
        "../Src/Examples/ConsoleVT.cpp"
    }
	
	-- project: example ConPTY 
project "Example_ConPTY"
    kind "ConsoleApp"
    setup_include_link_env()
	files {
        "../Src/Examples/EchoCon.cpp"
    }
	
	-- project: example ConPTY 
project "Example_ConsoleScroll"
    kind "ConsoleApp"
    setup_include_link_env()
	files {
        "../Src/Examples/ConsoleScrolling.cpp"
    }
	
	-- project: example debuggee 
project "Example_Debuggee"
    kind "ConsoleApp"
    setup_include_link_env()
	files {
        "../Src/Examples/Debuggee.cpp"
    }
	
	-- project: example vectored exception handler
project "Example_VectoredExceptionHandler"
    kind "ConsoleApp"
    setup_include_link_env()
	files {
        "../Src/Examples/VectoredExceptionHandler.cpp"
    }	
	
	-- stack buffer overflow
project "Example_BufferOverflow"
    kind "ConsoleApp"
    setup_include_link_env()
	files {
        "../Src/Examples/BufferOverflow.cpp"
    }
	
	-- SEH Raw
project "Example_SEH_Raw"
    kind "ConsoleApp"
    setup_include_link_env()
	files {
        "../Src/Examples/SEH_Raw.cpp"
    }
	
	-- SEH & cpp exception
project "Example_SEH_CPP"
    kind "ConsoleApp"
    setup_include_link_env()
	files {
        "../Src/Examples/SEH_CPP.cpp"
    }
	
	-- Performance View 
project "Example_PerfView"
    kind "ConsoleApp"
    setup_include_link_env()
	files {
        "../Src/Examples/PerfView.cpp"
    }
	
	-- project: WinDebugger
project "WinDebugger"
    kind "ConsoleApp"
    setup_include_link_env()
	files {
		"../Src/Foundation/AppHelper.h",
		"../Src/Foundation/AppHelper.cpp",
		"../Src/WinDebugger/WinProcessHelper.h",
		"../Src/WinDebugger/WinProcessHelper.cpp",
		"../Src/WinDebugger/WinDebugger.h",
		"../Src/WinDebugger/WinDebugger.cpp",
        "../Src/WinDebugger/Main.cpp"
    }	