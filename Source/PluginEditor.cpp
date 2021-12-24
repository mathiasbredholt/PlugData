/*
 // Copyright (c) 2021 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/


#include "PluginEditor.h"
#include "Canvas.h"
#include "Edge.h"
#include "Connection.h"
#include "Pd/x_libpd_mod_utils.h"
#include "Dialogs.h"

//==============================================================================

PlugDataPluginEditor::PlugDataPluginEditor(PlugDataAudioProcessor& p, Console* debugConsole) : AudioProcessorEditor(&p), pd(p), levelmeter(p.parameters)
{
    console = debugConsole;
    
    tabbar.setColour(TabbedButtonBar::frontOutlineColourId, MainLook::firstBackground);
    tabbar.setColour(TabbedButtonBar::tabOutlineColourId, MainLook::firstBackground);
    tabbar.setColour(TabbedComponent::outlineColourId, MainLook::firstBackground);
    
    setLookAndFeel(&mainLook);
    
    addAndMakeVisible(levelmeter);
    
    tabbar.onTabChange = [this](int idx) {
        Edge::connectingEdge = nullptr;
        
        if(idx == -1) return;
        
        // update GraphOnParent when changing tabs
        for(auto* box : getCurrentCanvas()->boxes) {
            if(box->graphics && box->graphics->getGUI().getType() == pd::Type::GraphOnParent) {
                auto* cnv = box->graphics.get()->getCanvas();
                if(cnv) cnv->synchronise();
            }
            if(box->graphics && (box->graphics->getGUI().getType() == pd::Type::Subpatch || box->graphics->getGUI().getType() == pd::Type::GraphOnParent)) {
                box->updatePorts();
            }
        }
        
        auto* cnv = getCurrentCanvas();
        if(cnv->patch.getPointer()) {
            cnv->patch.setCurrent();
        }

        timerCallback();
    };
    
    startButton.setClickingTogglesState(true);
    startButton.setConnectedEdges(12);
    startButton.setLookAndFeel(&statusbarLook);
    addAndMakeVisible(startButton);

    
    addAndMakeVisible(tabbar);
    addAndMakeVisible(console);
    
    addChildComponent(inspector);
    
    lockButton.setClickingTogglesState(true);
    lockButton.setConnectedEdges(12);
    lockButton.setLookAndFeel(&statusbarLook2);
    lockButton.onClick = [this](){
        pd.locked = lockButton.getToggleState();
        
        lockButton.setButtonText(lockButton.getToggleState() ? CharPointer_UTF8 ("\xef\x80\xa3") : CharPointer_UTF8("\xef\x82\x9c"));
        
        sendChangeMessage();
    };
    addAndMakeVisible(lockButton);
    
    connectionStyleButton.setClickingTogglesState(true);
    connectionStyleButton.setConnectedEdges(12);
    connectionStyleButton.setLookAndFeel(&statusbarLook);
    connectionStyleButton.onClick = [this](){
        pd.mainTree.setProperty(Identifiers::connectionStyle, connectionStyleButton.getToggleState(), nullptr);
        for(auto* connection : getCurrentCanvas()->connections) connection->resized();
    };
    
    addAndMakeVisible(connectionStyleButton);

    startButton.onClick = [this]() {
        pd.setBypass(!startButton.getToggleState());
    };
    
    startButton.setToggleState(true, dontSendNotification);
    
    for(auto& button : toolbarButtons) {
        button.setLookAndFeel(&toolbarLook);
        button.setConnectedEdges(12);
        addAndMakeVisible(button);
    }
    
    // New button
    toolbarButtons[0].onClick = [this]() {
        auto createFunc = [this](){
            tabbar.clearTabs();
            canvases.clear();
            auto* cnv = canvases.add(new Canvas(*this, false));
            cnv->title = "Untitled Patcher";
            mainCanvas = cnv;
            mainCanvas->createPatch();
            addTab(cnv);
        };

        
        if(getMainCanvas()->changed()) {
            SaveDialog::show(this, [this, createFunc](int result){
                if(result == 2) {
                    saveProject([this, createFunc]() mutable {
                        createFunc();
                    });
                }
                else if(result == 1) {
                    createFunc();
                }
            });
        }
        else {
            createFunc();
        }
    };
    
    // Open button
    toolbarButtons[1].onClick = [this]() {
        openProject();
    };
    
    // Save button
    toolbarButtons[2].onClick = [this]() {
        saveProject();
    };
    
    //  Undo button
    toolbarButtons[3].onClick = [this]() {
       
        getCurrentCanvas()->undo();
    };
    
    // Redo button
    toolbarButtons[4].onClick = [this]() {
        getCurrentCanvas()->redo();
    };
    
    // New object button
    toolbarButtons[5].onClick = [this]() {
        PopupMenu menu;
        menu.addItem (15, "Empty Object");
        menu.addSeparator();
        
        menu.addItem (1, "Numbox");
        menu.addItem (2, "Message");
        menu.addItem (3, "Bang");
        menu.addItem (4, "Toggle");
        menu.addItem (5, "Horizontal Slider");
        menu.addItem (6, "Vertical Slider");
        menu.addItem (7, "Horizontal Radio");
        menu.addItem (8, "Vertical Radio");
        
        menu.addSeparator();
        
        menu.addItem (11, "Float Atom");
        menu.addItem (12, "Symbol Atom");
        
        menu.addSeparator();
        
        menu.addItem (9, "Array");
        menu.addItem (13, "GraphOnParent");
        menu.addItem (14, "Comment");
        menu.addItem (10, "Canvas");
        
        
        auto callback = [this](int choice) {
            if(choice < 1) return;
            
            
            String boxName;
            
            switch (choice) {
                case 1:
                boxName = "nbx";
                break;
                
                case 2:
                boxName = "msg";
                break;
                
                case 3:
                boxName = "bng";
                break;
                
                case 4:
                boxName = "tgl";
                break;
                
                case 5:
                boxName = "hsl";
                break;
                
                case 6:
                boxName = "vsl";
                break;
                
                case 7:
                boxName = "hradio";
                break;
                
                case 8:
                boxName = "vradio";
                break;
                
                case 9: {
                    ArrayDialog::show(this, [this](int result, String name, String size) {
                        if(result) {
                            auto* cnv = getCurrentCanvas();
                            auto* box = new Box(cnv, "graph " + name + " " + size);
                            cnv->boxes.add(box);
                        }
                    });
                    return;
                }
                
               
                
                case 10:
                boxName = "canvas";
                break;
                    
                case 11:
                boxName = "floatatom";
                break;
                    
                case 12:
                boxName = "symbolatom";
                break;
                    
                case 13:
                boxName = "graph";
                break;
                    
                case 14:
                boxName = "comment";
                break;
                    

            }
            auto* cnv = getCurrentCanvas();
            cnv->boxes.add(new Box(cnv, boxName));
        };
        
        menu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent (toolbarButtons[5]), ModalCallbackFunction::create(callback));
    };
    
    hideButton.setLookAndFeel(&toolbarLook);
    hideButton.setClickingTogglesState(true);
    hideButton.setColour(ComboBox::outlineColourId, findColour(TextButton::buttonColourId));
    hideButton.setConnectedEdges(12);
    
    //  Open console
    toolbarButtons[7].onClick = [this]() {
        console->setVisible(true);
        inspector.setVisible(false);
    };
    
    // Open inspector
    toolbarButtons[8].onClick = [this]() {
        console->setVisible(false);
        inspector.setVisible(true);
    };
    
    hideButton.onClick = [this](){
        sidebarHidden = hideButton.getToggleState();
        hideButton.setButtonText(sidebarHidden ? CharPointer_UTF8("\xef\x81\x93") : CharPointer_UTF8("\xef\x81\x94"));
        
        toolbarButtons[7].setVisible(!sidebarHidden);
        toolbarButtons[8].setVisible(!sidebarHidden);
        
        repaint();
        resized();
    };
    
    addAndMakeVisible(hideButton);
    
    restrainer.setSizeLimits (150, 150, 2000, 2000);
    resizer.reset(new ResizableCornerComponent (this, &restrainer));
    addAndMakeVisible(resizer.get());
    
    setSize (pd.lastUIWidth, pd.lastUIHeight);

}

PlugDataPluginEditor::~PlugDataPluginEditor()
{
    setLookAndFeel(nullptr);
    startButton.setLookAndFeel(nullptr);
    hideButton.setLookAndFeel(nullptr);
    
    for(auto& button : toolbarButtons) {
        button.setLookAndFeel(nullptr);
    }
}


//==============================================================================
void PlugDataPluginEditor::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
    
    auto baseColour = MainLook::firstBackground;
    auto highlightColour = MainLook::highlightColour;
    
    int sWidth = sidebarHidden ? dragbarWidth : std::max(dragbarWidth, sidebarWidth);
    
    // Sidebar
    g.setColour(baseColour.darker(0.1));
    g.fillRect(getWidth() - sWidth, dragbarWidth, sWidth + 10, getHeight() - toolbarHeight);
    
    // Toolbar background
    g.setColour(baseColour);
    g.fillRect(0, 0, getWidth(), toolbarHeight - 4);
    
    g.setColour(highlightColour);
    g.drawRoundedRectangle({-4.0f, 39.0f, (float)getWidth() + 9, 20.0f}, 12.0, 4.0);
    
    // Make sure we cant see the bottom half of the rounded rectangle
    g.setColour(baseColour);
    g.fillRect(0, toolbarHeight - 4, getWidth(), toolbarHeight + 16);
    
    // Statusbar background
    g.setColour(baseColour);
    g.fillRect(0, getHeight() - statusbarHeight, getWidth(), statusbarHeight);
    
    // Draggable bar
    g.setColour(baseColour);
    g.fillRect(getWidth() - sWidth, dragbarWidth, statusbarHeight, getHeight() - (toolbarHeight - statusbarHeight));
    
    
}

void PlugDataPluginEditor::resized()
{
    int sWidth = sidebarHidden ? dragbarWidth : std::max(dragbarWidth, sidebarWidth);
    
    int sContentWidth = sWidth - dragbarWidth;
    
    int sbarY = toolbarHeight - 4;
    
    console->setBounds(getWidth() - sContentWidth, sbarY + 2, sContentWidth, getHeight() - sbarY);
    console->toFront(false);
    
    inspector.setBounds(getWidth() - sContentWidth, sbarY + 2, sContentWidth, getHeight() - sbarY);
    inspector.toFront(false);
    
    tabbar.setBounds(0, sbarY, getWidth() - sWidth, getHeight() - sbarY - statusbarHeight);
    tabbar.toFront(false);
    
    startButton.setBounds(getWidth() - sWidth - 40, getHeight() - 27, 27, 27);
    
    
    int jumpPositions[2] = {3, 5};
    int idx = 0;
    int toolbarPosition = 0;
    for(int b = 0; b < 7; b++) {
        auto& button = toolbarButtons[b];
        int spacing = (25 * (idx >= jumpPositions[0])) +  (25 * (idx >= jumpPositions[1])) + 10;
        button.setBounds(toolbarPosition + spacing, 0, 70, toolbarHeight);
        toolbarPosition += 70;
        idx++;
    }
    
    hideButton.setBounds(std::min(getWidth() - sWidth, getWidth() - 80), 0, 70, toolbarHeight);
    toolbarButtons[7].setBounds(std::min(getWidth() - sWidth + 90, getWidth() - 80), 0, 70, toolbarHeight);
    toolbarButtons[8].setBounds(std::min(getWidth() - sWidth + 160, getWidth() - 80), 0, 70, toolbarHeight);
    
    lockButton.setBounds(8, getHeight() - 27, 27, 27);
    connectionStyleButton.setBounds(38, getHeight() - 27, 27, 27);
    
    levelmeter.setBounds(getWidth() - sWidth - 150, getHeight() - 27, 100, 27);
    
    resizer->setBounds (getWidth() - 16, getHeight() - 16, 16, 16);
    resizer->toFront(false);
    
    pd.lastUIWidth = getWidth();
    pd.lastUIHeight = getHeight();
}

void PlugDataPluginEditor::mouseDown(const MouseEvent &e) {
    Rectangle<int> drag_bar(getWidth() - sidebarWidth, dragbarWidth, sidebarWidth, getHeight() - toolbarHeight);
    if(drag_bar.contains(e.getPosition()) && !sidebarHidden) {
        draggingSidebar = true;
        dragStartWidth = sidebarWidth;
    }
    else {
        draggingSidebar = false;
    }
    
   
}

void PlugDataPluginEditor::mouseDrag(const MouseEvent &e) {
    if(draggingSidebar) {
        sidebarWidth = dragStartWidth - e.getDistanceFromDragStartX();
        repaint();
        resized();
    }
}

void PlugDataPluginEditor::mouseUp(const MouseEvent &e) {
    draggingSidebar = false;
}


void PlugDataPluginEditor::openProject() {
        
    auto openFunc = [this](const FileChooser& f) {
        File openedFile = f.getResult();
        
        if(openedFile.exists() && openedFile.getFileExtension().equalsIgnoreCase(".pd")) {
            tabbar.clearTabs();
            pd.loadPatch(openedFile.getFullPathName());
        }
    };
    
    if(getMainCanvas()->changed()) {
        
        SaveDialog::show(this, [this, openFunc](int result){
            if(result == 2) {
                saveProject([this, openFunc]() mutable {
                    openChooser.launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, openFunc);
                }); 
            }
            else if(result != 0) {
                openChooser.launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, openFunc);
            }
        });
    }
    else {
        openChooser.launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, openFunc);
    }
}

void PlugDataPluginEditor::saveProject(std::function<void()> nestedCallback) {
    auto to_save = pd.getCanvasContent();

    saveChooser.launchAsync(FileBrowserComponent::saveMode | FileBrowserComponent::warnAboutOverwriting, [this, to_save, nestedCallback](const FileChooser &f) mutable {
        
        File result = saveChooser.getResult();
        
        FileOutputStream ostream(result);
        ostream.writeString(to_save);
        
        getCurrentCanvas()->title = result.getFileName();
        
        nestedCallback();
    });
    
    getCurrentCanvas()->hasChanged = false;
}

void PlugDataPluginEditor::timerCallback() {
    auto* cnv = getCurrentCanvas();
    //cnv->patch.setCurrent();

    
    for(auto& box : cnv->boxes) {
        if(box->graphics && box->isShowing()) {
            box->graphics->updateValue();
        }
    }
    
    updateUndoState();
    
   // pd.getCallbackLock()->exit();
}

void PlugDataPluginEditor::updateUndoState() {
    
    pd.setThis();
    
    toolbarButtons[5].setEnabled(!pd.locked);

    
    if(getCurrentCanvas() && getCurrentCanvas()->patch.getPointer() && !pd.locked) {
        //auto* cs = pd.getCallbackLock();
        getCurrentCanvas()->patch.setCurrent();

        getCurrentCanvas()->hasChanged = true;
        
        toolbarButtons[3].setEnabled(pd.canUndo);
        toolbarButtons[4].setEnabled(pd.canRedo);
    }
    else {
        toolbarButtons[3].setEnabled(false);
        toolbarButtons[4].setEnabled(false);
    }
}

Canvas* PlugDataPluginEditor::getCurrentCanvas()
{
    if(auto* viewport = dynamic_cast<Viewport*>(tabbar.getCurrentContentComponent())) {
        if(auto* cnv = dynamic_cast<Canvas*>(viewport->getViewedComponent()->getChildComponent(0))) {
            return cnv;
        }
    }
    return nullptr;
}

Canvas* PlugDataPluginEditor::getMainCanvas() {
    return mainCanvas;
}

Canvas* PlugDataPluginEditor::getCanvas(int idx) {
    if(auto* viewport = dynamic_cast<Viewport*>(tabbar.getTabContentComponent(idx))) {
        if(auto* cnv = dynamic_cast<Canvas*>(viewport->getViewedComponent()->getChildComponent(0))) {
            return cnv;
        }
    }
    
    return nullptr;
}


void PlugDataPluginEditor::addTab(Canvas* cnv)
{
    tabbar.addTab(cnv->title, findColour(ResizableWindow::backgroundColourId), cnv->viewport, true);
    
    int tabIdx = tabbar.getNumTabs() - 1;
    tabbar.setCurrentTabIndex(tabIdx);
    
    if(tabbar.getNumTabs() > 1) {
        tabbar.getTabbedButtonBar().setVisible(true);
        tabbar.setTabBarDepth(30);
    }
    else {
        tabbar.getTabbedButtonBar().setVisible(false);
        tabbar.setTabBarDepth(1);
    }
    
    auto* tabButton = tabbar.getTabbedButtonBar().getTabButton(tabIdx);
    
    auto* closeButton = new TextButton("x");
   
    closeButton->onClick = [this, tabButton]() mutable {
        
        // We cant use the index from earlier because it might change!
        int idx = -1;
        for(int i = 0; i < tabbar.getNumTabs(); i++) {
            if(tabbar.getTabbedButtonBar().getTabButton(i) == tabButton) {
                idx = i;
                break;
            }
        }
        
        if(idx == -1) return;
        
        if(tabbar.getCurrentTabIndex() == idx) {
            tabbar.setCurrentTabIndex(0, false);
        }
        
        auto* cnv = getCanvas(idx);
        canvases.removeObject(cnv);
        tabbar.removeTab(idx);
        
        tabbar.setCurrentTabIndex(0, true);
        
        if(tabbar.getNumTabs() == 1) {
            tabbar.getTabbedButtonBar().setVisible(false);
            tabbar.setTabBarDepth(1);
        }
    };
    
    closeButton->setColour(TextButton::buttonColourId, Colour());
    closeButton->setColour(TextButton::buttonOnColourId, Colour());
    closeButton->setColour(ComboBox::outlineColourId, Colour());
    closeButton->setColour(TextButton::textColourOnId, Colours::white);
    closeButton->setColour(TextButton::textColourOffId, Colours::white);
    closeButton->setConnectedEdges(12);
    tabButton->setExtraComponent(closeButton, TabBarButton::beforeText);
    
    closeButton->setVisible(tabIdx != 0);
    closeButton->setSize(28, 28);
    
    tabbar.repaint();
    
    cnv->setVisible(true);
    //cnv->setBounds(0, 0, 1000, 700);
}
