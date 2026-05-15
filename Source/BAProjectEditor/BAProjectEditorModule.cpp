// Copyright TeamBA. All Rights Reserved.

#include "BAProjectEditorModule.h"

#include "BATableGenerator.h"
#include "Framework/Commands/UIAction.h"
#include "ToolMenus.h"
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

			DataSection.AddMenuEntry(
				"GenerateTableData",
				LOCTEXT("GenerateTableDataLabel", "GenerateTableData"),
				LOCTEXT("GenerateTableDataTooltip",
					"Run the Excel -> JSON converter and rebuild every DataTable under /Game/Table."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FBATableGenerator::Generate)));
			
			// Reload
			DataSection.AddMenuEntry(
				"ReloadTableData",
				LOCTEXT("ReloadTableDataLabel", "ReloadTableData"),
				LOCTEXT("ReloadTableDataTooltip",
					"Reload every DataTable under /Game/Table."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([]()
				{
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
		}));

	SubMenuEntry.InsertPosition = FToolMenuInsert("Tools", EToolMenuInsertType::After);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FBAProjectEditorModule, BAProjectEditor);
