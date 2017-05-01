##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=MeshNetwork
ConfigurationName      :=Debug
WorkspacePath          :=/home/ilie/workspace/PubSub
ProjectPath            :=/home/ilie/workspace/PubSub/MeshNetwork
IntermediateDirectory  :=./Debug
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=Ilie Danila
Date                   :=01/05/17
CodeLitePath           :=/home/ilie/.codelite
LinkerName             :=/usr/bin/clang++
SharedObjectLinkerName :=/usr/bin/clang++ -shared -fPIC
ObjectSuffix           :=.o
DependSuffix           :=.o.d
PreprocessSuffix       :=.i
DebugSwitch            :=-g 
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
ObjectsFileList        :="MeshNetwork.txt"
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
AR       := /usr/bin/llvm-ar rcu
CXX      := /usr/bin/clang++
CC       := /usr/bin/clang
CXXFLAGS :=  -g -std=c++14  $(Preprocessors)
CFLAGS   :=  -g $(Preprocessors)
ASFLAGS  := 
AS       := /usr/bin/llvm-as


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=$(IntermediateDirectory)/src_connection.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_node.cpp$(ObjectSuffix) 



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
	@$(MakeDirCommand) "/home/ilie/workspace/PubSub/.build-debug"
	@echo rebuilt > "/home/ilie/workspace/PubSub/.build-debug/MeshNetwork"

MakeIntermediateDirs:
	@test -d ./Debug || $(MakeDirCommand) ./Debug


./Debug:
	@test -d ./Debug || $(MakeDirCommand) ./Debug

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/src_connection.cpp$(ObjectSuffix): src/connection.cpp $(IntermediateDirectory)/src_connection.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/ilie/workspace/PubSub/MeshNetwork/src/connection.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_connection.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_connection.cpp$(DependSuffix): src/connection.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_connection.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_connection.cpp$(DependSuffix) -MM src/connection.cpp

$(IntermediateDirectory)/src_connection.cpp$(PreprocessSuffix): src/connection.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_connection.cpp$(PreprocessSuffix) src/connection.cpp

$(IntermediateDirectory)/src_node.cpp$(ObjectSuffix): src/node.cpp $(IntermediateDirectory)/src_node.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/ilie/workspace/PubSub/MeshNetwork/src/node.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_node.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_node.cpp$(DependSuffix): src/node.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_node.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_node.cpp$(DependSuffix) -MM src/node.cpp

$(IntermediateDirectory)/src_node.cpp$(PreprocessSuffix): src/node.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_node.cpp$(PreprocessSuffix) src/node.cpp


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) -r ./Debug/


