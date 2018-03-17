##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Release
ProjectName            :=MeshNetwork
ConfigurationName      :=Release
WorkspacePath          :=/home/ilie/workspace/AsyncPubSub
ProjectPath            :=/home/ilie/workspace/AsyncPubSub/MeshNetwork
IntermediateDirectory  :=./Release
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=Ilie Danila
Date                   :=28/10/17
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
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). $(IncludeSwitch)../boost/inc 
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
CXXFLAGS :=  -std=c++14  $(Preprocessors)
CFLAGS   :=   $(Preprocessors)
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
	@$(MakeDirCommand) "/home/ilie/workspace/AsyncPubSub/.build-release"
	@echo rebuilt > "/home/ilie/workspace/AsyncPubSub/.build-release/MeshNetwork"

MakeIntermediateDirs:
	@test -d ./Release || $(MakeDirCommand) ./Release


./Release:
	@test -d ./Release || $(MakeDirCommand) ./Release

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/src_connection.cpp$(ObjectSuffix): src/connection.cpp $(IntermediateDirectory)/src_connection.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/ilie/workspace/AsyncPubSub/MeshNetwork/src/connection.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_connection.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_connection.cpp$(DependSuffix): src/connection.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_connection.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_connection.cpp$(DependSuffix) -MM src/connection.cpp

$(IntermediateDirectory)/src_connection.cpp$(PreprocessSuffix): src/connection.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_connection.cpp$(PreprocessSuffix) src/connection.cpp

$(IntermediateDirectory)/src_node.cpp$(ObjectSuffix): src/node.cpp $(IntermediateDirectory)/src_node.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/ilie/workspace/AsyncPubSub/MeshNetwork/src/node.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_node.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_node.cpp$(DependSuffix): src/node.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_node.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_node.cpp$(DependSuffix) -MM src/node.cpp

$(IntermediateDirectory)/src_node.cpp$(PreprocessSuffix): src/node.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_node.cpp$(PreprocessSuffix) src/node.cpp


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) -r ./Release/


