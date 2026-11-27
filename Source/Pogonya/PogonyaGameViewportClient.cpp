#include "PogonyaGameViewportClient.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "PogonyaMainMenu"

void UPogonyaGameViewportClient::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    UWorld* CurrentWorld = GetWorld();
    if (!CurrentWorld || CheckedWorld.Get() == CurrentWorld)
    {
        return;
    }

    if (MainMenu.IsValid())
    {
        RemoveViewportWidgetContent(MainMenu.ToSharedRef());
        MainMenu.Reset();
        MenuController.Reset();
    }

    const TCHAR* CommandLine = FCommandLine::Get();
    if (!FApp::CanEverRender() || CurrentWorld->GetNetMode() != NM_Standalone ||
        !CurrentWorld->GetMapName().EndsWith(TEXT("L_Sector04")) ||
        FParse::Param(CommandLine, TEXT("PogonyaBlueprintSelfTest")) ||
        FParse::Param(CommandLine, TEXT("PogonyaBlueprintPreview")))
    {
        CheckedWorld = CurrentWorld;
        return;
    }

    APlayerController* Controller = CurrentWorld->GetFirstPlayerController();
    if (!Viewport || !Controller || !Controller->IsLocalController() || !CurrentWorld->HasBegunPlay())
    {
        return;
    }

    CheckedWorld = CurrentWorld;
    if (Controller->SetPause(true))
    {
        ShowMainMenu(*Controller);
    }
}

void UPogonyaGameViewportClient::ShowMainMenu(APlayerController& Controller)
{
    MenuController = &Controller;
    bPreviousCursorVisibility = Controller.bShowMouseCursor;

    TSharedPtr<SWidgetSwitcher> Pages;
    TSharedPtr<SButton> PlayButton;
    TSharedPtr<SButton> BackButton;

    MainMenu = SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(FLinearColor(0.018f, 0.027f, 0.04f))
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Center)
        [
            SAssignNew(Pages, SWidgetSwitcher)
            + SWidgetSwitcher::Slot()
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("Title", "ПОГОНЯ"))
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 46))
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 32)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("Sector", "СЕКТОР 04"))
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
                    .ColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.58f, 0.2f)))
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
                [
                    SAssignNew(PlayButton, SButton)
                    .HAlign(HAlign_Center).ContentPadding(FMargin(24, 12))
                    .Text(LOCTEXT("Play", "Играть"))
                    .OnClicked_Lambda([this]()
                    {
                        StartPlaying();
                        return FReply::Handled();
                    })
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
                [
                    SNew(SButton)
                    .HAlign(HAlign_Center).ContentPadding(FMargin(24, 12))
                    .Text(LOCTEXT("Controls", "Управление"))
                    .OnClicked_Lambda([this]()
                    {
                        const TSharedPtr<SWidgetSwitcher> Switcher = MenuPages.Pin();
                        const TSharedPtr<SButton> Back = MenuBackButton.Pin();
                        Switcher->SetActiveWidgetIndex(1);
                        return FReply::Handled().SetUserFocus(Back.ToSharedRef());
                    })
                ]
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(SButton)
                    .HAlign(HAlign_Center).ContentPadding(FMargin(24, 12))
                    .Text(LOCTEXT("Quit", "Выход"))
                    .OnClicked_Lambda([this]()
                    {
                        if (APlayerController* Player = MenuController.Get())
                        {
                            UKismetSystemLibrary::QuitGame(this, Player, EQuitPreference::Quit, false);
                        }
                        return FReply::Handled();
                    })
                ]
            ]
            + SWidgetSwitcher::Slot()
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("ControlsTitle", "Управление"))
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 24))
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 20)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("Bindings", "WASD — движение\nМышь — обзор\nShift — бег\nПробел — прыжок\nE — взаимодействие\nЛевая кнопка мыши — удар\nR — новый раунд"))
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
                ]
                + SVerticalBox::Slot().AutoHeight()
                [
                    SAssignNew(BackButton, SButton)
                    .HAlign(HAlign_Center).ContentPadding(FMargin(24, 12))
                    .Text(LOCTEXT("Back", "Назад"))
                    .OnClicked_Lambda([this]()
                    {
                        const TSharedPtr<SWidgetSwitcher> Switcher = MenuPages.Pin();
                        const TSharedPtr<SButton> Play = MenuPlayButton.Pin();
                        Switcher->SetActiveWidgetIndex(0);
                        return FReply::Handled().SetUserFocus(Play.ToSharedRef());
                    })
                ]
            ]
        ];

    MenuPages = Pages;
    MenuPlayButton = PlayButton;
    MenuBackButton = BackButton;
    AddViewportWidgetContent(MainMenu.ToSharedRef(), 100);
    FInputModeUIOnly InputMode;
    InputMode.SetWidgetToFocus(PlayButton);
    Controller.SetInputMode(InputMode);
    Controller.bShowMouseCursor = true;
}

void UPogonyaGameViewportClient::StartPlaying()
{
    APlayerController* Controller = MenuController.Get();
    if (!Controller || !Controller->SetPause(false))
    {
        return;
    }

    Controller->bShowMouseCursor = bPreviousCursorVisibility;
    Controller->SetInputMode(FInputModeGameOnly());
    RemoveViewportWidgetContent(MainMenu.ToSharedRef());
    MainMenu.Reset();
    MenuController.Reset();
}

#undef LOCTEXT_NAMESPACE
