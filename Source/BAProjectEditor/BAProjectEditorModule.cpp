// Copyright TeamBA. All Rights Reserved.

#include "BAProjectEditorModule.h"

#include "BABTAnalyzer.h"
#include "BAAIExporter.h"
#include "BATableGenerator.h"
#include "EditorUtilityLibrary.h"
#include "Framework/Commands/UIAction.h"
#include "ToolMenus.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Tables/BATableManager.h"
#include "Textures/SlateIcon.h"

#define LOCTEXT_NAMESPACE "BAProjectEditor"

void FBAProjectEditorModule::StartupModule()
{
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FBAProjectEditorModule::RegisterMenus));
}

void FBAProjectEditorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
}

void FBAProjectEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* MainMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu");
	if (!MainMenu)
	{
		return;
	}

	FToolMenuSection& Section = MainMenu->FindOrAddSection(NAME_None);

	FToolMenuEntry& SubMenuEntry = Section.AddSubMenu(
		"TeamBA",
		LOCTEXT("TeamBALabel", "TeamBA"),
		LOCTEXT("TeamBATooltip", "TeamBA editor tools"),
		FNewToolMenuDelegate::CreateLambda([](UToolMenu* InSubMenu)
		{
			FToolMenuSection& DataSection = InSubMenu->AddSection(
				"Data", LOCTEXT("DataSection", "Data"));

			// GenerateTableData
			/*DataSection.AddMenuEntry(
				"GenerateTableData",
				LOCTEXT("GenerateTableDataLabel", "GenerateTableData"),
				LOCTEXT("GenerateTableDataTooltip",
					"Run the Excel -> JSON converter and rebuild every DataTable under /Game/Table."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FBATableGenerator::Generate)));*/
			
			// Reload
			DataSection.AddMenuEntry(
				"GenerateAndReloadTableData",
				LOCTEXT("ReloadTableDataLabel", "Generate & Reload Table Data"),
				LOCTEXT("ReloadTableDataTooltip",
					"Reload every DataTable under /Game/Table."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([]()
				{
					// Generate
					FBATableGenerator::Generate();
					// Reload
					if (!GEngine)
					{
						UE_LOG(LogTemp, Warning, TEXT("GEngine is null, cannot reload table data."));
						return;
					}
					UBATableManager* TableManager = GEngine->GetEngineSubsystem<UBATableManager>();
					if (!TableManager)
					{
						UE_LOG(LogTemp, Warning, TEXT("UBATableManager is null, cannot reload table data."));
						return;
					}
					TableManager->ReloadAllTables();
					UE_LOG(LogTemp, Log, TEXT("Table data reloaded successfully."));
				})));

			FToolMenuSection& AISection = InSubMenu->AddSection(
				"AI", LOCTEXT("AISection", "AI"));

			// Analyze AI
			AISection.AddMenuEntry(
				"AnalyzeAI",
				LOCTEXT("AnalyzeAILabel", "Analyze Selected BT"),
				LOCTEXT("AnalyzeAITooltip", "Analyze selected Behavior Tree and export to Mermaid/D2"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.BehaviorTree"),
				FUIAction(FExecuteAction::CreateLambda([]()
				{
					TArray<UObject*> SelectedAssets = UEditorUtilityLibrary::GetSelectedAssets();
					if (SelectedAssets.Num() == 0)
					{
						UE_LOG(LogTemp, Warning, TEXT("No assets selected. Please select a Behavior Tree in the Content Browser."));
						return;
					}

					UBABTAnalyzer* Analyzer = NewObject<UBABTAnalyzer>();
					for (UObject* Asset : SelectedAssets)
					{
						UBehaviorTree* BT = Cast<UBehaviorTree>(Asset);
						if (!BT)
						{
							continue;
						}

						FBAAIAnalyzerTreeData TreeData;
						if (Analyzer->AnalyzeBehaviorTree(BT, TreeData))
						{
							FString MermaidContent;
							if (FBAAIExporter::ExportToMermaid(TreeData, MermaidContent))
							{
								FBAAIExporter::SaveToFile(FString::Printf(TEXT("%s.mmd"), *BT->GetName()), MermaidContent);
							}

							FString D2Content;
							if (FBAAIExporter::ExportToD2(TreeData, D2Content))
							{
								FBAAIExporter::SaveToFile(FString::Printf(TEXT("%s.d2"), *BT->GetName()), D2Content);
							}
						}
					}
				})));
		}));

	SubMenuEntry.InsertPosition = FToolMenuInsert("Tools", EToolMenuInsertType::After);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FBAProjectEditorModule, BAProjectEditor);
