include(CSharpUtilities)

# set architecture var (x86 or x64)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(ARCH x64)
	set(CMAKE_CSharp_FLAGS "/platform:x64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
	set(ARCH x86)
	set(CMAKE_CSharp_FLAGS "/platform:x86")
endif()

set(CMAKE_DOTNET_TARGET_FRAMEWORK_VERSION "v4.0")

file(GLOB_RECURSE SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cs"
	)
  
add_library(worker_wpf SHARED ${SOURCES})

set_property(TARGET worker_wpf PROPERTY VS_PACKAGE_REFERENCES 
		"Newtonsoft.Json_13.0.1"
	)

set_property(TARGET worker_wpf PROPERTY VS_DOTNET_REFERENCES
		"Newtonsoft.Json"
		
		"PresentationCore" 
		"PresentationFramework" 
		"System" 
		"System.Core" 
		"System.Windows.Presentation" 
		"System.Xml.Linq" 
		"System.Data.DataSetExtensions" 
		"System.Data" 
		"System.Xml" 
		"Microsoft.CSharp" 
		"WindowsBase"
	) 
	
# install
set(INSTALL_DIR ${BACKEND_BIN_DIR}/${ARCH})
install(TARGETS worker_wpf 
		RUNTIME
		DESTINATION ${INSTALL_DIR})
install(FILES 
			"${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/Newtonsoft.Json.dll"
		DESTINATION ${INSTALL_DIR})