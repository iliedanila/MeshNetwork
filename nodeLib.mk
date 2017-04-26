##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=nodeLib
ConfigurationName      :=Debug
WorkspacePath          :=/home/ilie/workspace/nodes
ProjectPath            :=/home/ilie/workspace/nodes/nodeLib
IntermediateDirectory  :=./Debug
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=Ilie Danila
Date                   :=26/04/17
CodeLitePath           :=/home/ilie/.codelite
LinkerName             :=clang++
SharedObjectLinkerName :=clang++ -shared -fPIC
ObjectSuffix           :=.o
DependSuffix           :=
PreprocessSuffix       :=.o.i
DebugSwitch            :=-gstab
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
SourceSwitch           :=-c 
OutputFile             :=$(IntermediateDirectory)/lib$(ProjectName).a
Preprocessors          :=
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E 
ObjectsFileList        :="nodeLib.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). $(IncludeSwitch)/usr/include 
IncludePCH             := 
RcIncludePath          := 
Libs                   := 
ArLibs                 :=  
LibPath                := $(LibraryPathSwitch). 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := ar rcus
CXX      := clang++
CC       := clang
CXXFLAGS :=  -g -std=c++14  $(Preprocessors)
CFLAGS   :=  -g $(Preprocessors)
ASFLAGS  := 
AS       := llvm-as


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=$(IntermediateDirectory)/nodeLib_connection.cpp$(ObjectSuffix) $(IntermediateDirectory)/nodeLib_node.cpp$(ObjectSuffix) 



Objects=$(Objects0) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild MakeIntermediateDirs
all: $(IntermediateDirectory) $(OutputFile)

$(OutputFile): $(Objects)
	@$(MakeDirCommand) $(@D)
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	$(AR) $(ArchiveOutputSwitch)$(OutputFile) @$(ObjectsFileList) $(ArLibs)
	@$(MakeDirCommand) "/home/ilie/workspace/nodes/.build-debug"
	@echo rebuilt > "/home/ilie/workspace/nodes/.build-debug/nodeLib"

MakeIntermediateDirs:
	@test -d ./Debug || $(MakeDirCommand) ./Debug


./Debug:
	@test -d ./Debug || $(MakeDirCommand) ./Debug

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/nodeLib_connection.cpp$(ObjectSuffix): nodeLib/connection.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/ilie/workspace/nodes/nodeLib/nodeLib/connection.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/nodeLib_connection.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/nodeLib_connection.cpp$(PreprocessSuffix): nodeLib/connection.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/nodeLib_connection.cpp$(PreprocessSuffix) nodeLib/connection.cpp

$(IntermediateDirectory)/nodeLib_node.cpp$(ObjectSuffix): nodeLib/node.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/ilie/workspace/nodes/nodeLib/nodeLib/node.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/nodeLib_node.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/nodeLib_node.cpp$(PreprocessSuffix): nodeLib/node.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/nodeLib_node.cpp$(PreprocessSuffix) nodeLib/node.cpp

##
## Clean
##
clean:
	$(RM) -r ./Debug/


