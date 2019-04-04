<?php
require_once($argv[1]); // type.php
require_once($argv[2]); // program.php
$file_prefix = $argv[3];
$idl_format = $argv[4];
$dsn_root = getenv('DSN_ROOT');

function getGUID()
{
    if (function_exists('com_create_guid'))
    {
        return com_create_guid();
    }
    else
    {
        // Referenced from phunction (https://github.com/alixaxel/phunction, MIT License)
        // Commit: f52d50ae6a80aaea0a6b1339e3d0289d76a26ac4
        // Text.php, line 101-121
        $result = array();
        for ($i = 0; $i < 8; ++$i)
        {
            switch ($i)
            {
                case 3:
                    $result[$i] = mt_rand(16384, 20479);
                break;
                case 4:
                    $result[$i] = mt_rand(32768, 49151);
                break;
                default:
                    $result[$i] = mt_rand(0, 65535);
                break;
            }
        }
        return vsprintf('{%04X%04X-%04X-%04X-%04X-%04X%04X%04X}', $result);
    }
}

$appguid = getGUID();
echo "<?xml version=\"1.0\" encoding=\"utf-8\"?>".PHP_EOL;

?>
<Project ToolsVersion="12.0" DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <Import Project="$(MSBuildExtensionsPath)\$(MSBuildToolsVersion)\Microsoft.Common.props" Condition="Exists('$(MSBuildExtensionsPath)\$(MSBuildToolsVersion)\Microsoft.Common.props')" />
  <PropertyGroup>
    <Configuration Condition=" '$(Configuration)' == '' ">Release</Configuration>
    <Platform Condition=" '$(Platform)' == '' ">AnyCPU</Platform>
    <ProjectGuid><?=$appguid?></ProjectGuid>
    <OutputType>Exe</OutputType>
    <AppDesignerFolder>Properties</AppDesignerFolder>
    <RootNamespace><?=$_PROG->name?></RootNamespace>
    <AssemblyName><?=$_PROG->name?></AssemblyName>
    <TargetFrameworkVersion>v4.5</TargetFrameworkVersion>
    <FileAlignment>512</FileAlignment>
  </PropertyGroup>
  <PropertyGroup Condition=" '$(Configuration)|$(Platform)' == 'Debug|AnyCPU' ">
    <PlatformTarget>AnyCPU</PlatformTarget>
    <DebugSymbols>true</DebugSymbols>
    <DebugType>full</DebugType>
    <Optimize>false</Optimize>
    <OutputPath>${MY_OUTPUT_DIRECTORY}</OutputPath>
    <DefineConstants>DEBUG;TRACE</DefineConstants>
    <ErrorReport>prompt</ErrorReport>
    <WarningLevel>4</WarningLevel>
    <Prefer32Bit>false</Prefer32Bit>
  </PropertyGroup>
  <PropertyGroup Condition=" '$(Configuration)|$(Platform)' == 'Release|AnyCPU' ">
    <PlatformTarget>AnyCPU</PlatformTarget>
    <DebugType>pdbonly</DebugType>
    <Optimize>true</Optimize>
    <OutputPath>${MY_OUTPUT_DIRECTORY}</OutputPath>
    <DefineConstants>TRACE</DefineConstants>
    <ErrorReport>prompt</ErrorReport>
    <WarningLevel>4</WarningLevel>
    <Prefer32Bit>false</Prefer32Bit>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)' == 'Debug|x64'">
    <DebugSymbols>true</DebugSymbols>
    <OutputPath>${MY_OUTPUT_DIRECTORY}</OutputPath>
    <DefineConstants>DEBUG;TRACE</DefineConstants>
    <DebugType>full</DebugType>
    <PlatformTarget>x64</PlatformTarget>
    <ErrorReport>prompt</ErrorReport>
    <CodeAnalysisRuleSet>MinimumRecommendedRules.ruleset</CodeAnalysisRuleSet>
    <Prefer32Bit>true</Prefer32Bit>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)' == 'Release|x64'">
    <OutputPath>${MY_OUTPUT_DIRECTORY}</OutputPath>
    <DefineConstants>TRACE</DefineConstants>
    <Optimize>true</Optimize>
    <DebugType>pdbonly</DebugType>
    <PlatformTarget>x64</PlatformTarget>
    <ErrorReport>prompt</ErrorReport>
    <CodeAnalysisRuleSet>MinimumRecommendedRules.ruleset</CodeAnalysisRuleSet>
    <Prefer32Bit>true</Prefer32Bit>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)' == 'Debug|x86'">
    <DebugSymbols>true</DebugSymbols>
    <OutputPath>${MY_OUTPUT_DIRECTORY}</OutputPath>
    <DefineConstants>DEBUG;TRACE</DefineConstants>
    <DebugType>full</DebugType>
    <PlatformTarget>x86</PlatformTarget>
    <ErrorReport>prompt</ErrorReport>
    <CodeAnalysisRuleSet>MinimumRecommendedRules.ruleset</CodeAnalysisRuleSet>
    <Prefer32Bit>true</Prefer32Bit>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)' == 'Release|x86'">
    <OutputPath>${MY_OUTPUT_DIRECTORY}</OutputPath>
    <DefineConstants>TRACE</DefineConstants>
    <Optimize>true</Optimize>
    <DebugType>pdbonly</DebugType>
    <PlatformTarget>x86</PlatformTarget>
    <ErrorReport>prompt</ErrorReport>
    <CodeAnalysisRuleSet>MinimumRecommendedRules.ruleset</CodeAnalysisRuleSet>
    <Prefer32Bit>true</Prefer32Bit>
  </PropertyGroup>
  <ItemGroup>
    <Reference Include="System" />
    <Reference Include="System.Core" />
    <Reference Include="dsn.dev.csharp">
        <HintPath><?=$dsn_root?>\lib\dsn.dev.csharp.dll</HintPath>
    </Reference>
<?php if ($idl_format == "proto") { ?>
    <Reference Include="Google.Protobuf">
        <HintPath><?=$dsn_root?>\lib\Google.Protobuf.dll</HintPath>
    </Reference>
<?php } else if ($idl_format == "thrift") { ?>
    <Reference Include="Thrift">
        <HintPath><?=$dsn_root?>\lib\Thrift.dll</HintPath>
    </Reference>
<?php } ?>
  </ItemGroup>
  <ItemGroup>
    <Compile Include="${MY_PROJ_SRC}" />
  </ItemGroup>
  <ItemGroup>
    <None Include="${MY_CURRENT_SOURCE_DIR}\App.config" />
  </ItemGroup>
  <ItemGroup>
    <None Include="${MY_CURRENT_SOURCE_DIR}\config.ini">
      <CopyToOutputDirectory>PreserveNewest</CopyToOutputDirectory>
    </None>
  </ItemGroup>
  <Import Project="$(MSBuildToolsPath)\Microsoft.CSharp.targets" />
  <!-- To modify your build process, add your task inside one of the targets below and uncomment it. 
       Other similar extension points exist, see Microsoft.Common.targets.
  <Target Name="BeforeBuild">
  </Target>
  <Target Name="AfterBuild">
  </Target>
  -->
</Project>
