/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "Box.h"
#include "Canvas.h"
#include "Connection.h"
#include "Edge.h"
#include "PluginEditor.h"

#include "Pd/x_libpd_extra_utils.h"
#include "Pd/x_libpd_mod_utils.h"

//==============================================================================
Box::Box(Canvas* parent, String name, Point<int> position)
    : textLabel(this, parent->dragger)
    , dragger(parent->dragger)
{
    cnv = parent;

    initialise();
    setTopLeftPosition(position);
    setType(name);

    // Add listener for lock/unlock messages
    cnv->main.addChangeListener(this);

    // Open editor for undefined objects
    if (name.isEmpty()) {
        textLabel.showEditor();
    }

    // Updates lock/unlock mode
    changeListenerCallback(nullptr);
}

Box::Box(pd::Object* object, Canvas* parent, String name, Point<int> position)
    : pdObject(object)
    , textLabel(this, parent->dragger)
    , dragger(parent->dragger)
{
    cnv = parent;
    initialise();
    setTopLeftPosition(position);
    setType(name, true);

    // Add listener for lock/unlock messages
    cnv->main.addChangeListener(this);

    // Updates lock/unlock mode
    changeListenerCallback(nullptr);
}

Box::~Box()
{
    cnv->main.removeChangeListener(this);
}

void Box::changeListenerCallback(ChangeBroadcaster* source)
{
    // Called when locking/unlocking

    // If the object has graphics, we hide the draggable name object
    if (graphics && !graphics->fakeGUI() && (cnv->pd->locked || cnv->isGraph)) {
        locked = true;
        textLabel.setVisible(false);
        if (resizer)
            resizer->setVisible(false);

        auto [w, h] = graphics->getBestSize();
        graphics->setSize(w, h);
        setSize(w + 8, h + 8);
    } else {
        locked = false;
        textLabel.setVisible(true);
        if (resizer)
            resizer->setVisible(true);

        if (graphics && !graphics->fakeGUI()) {
            auto [w, h] = graphics->getBestSize();
            graphics->setSize(w, h);
            setSize(w + 8, h + 29);
        }
    }
}

void Box::initialise()
{

    addMouseListener(cnv, true); // Receive mouse messages on canvas
    cnv->addAndMakeVisible(this);

    setSize(55, 31);

    addAndMakeVisible(&textLabel);

    textLabel.toBack();

    textLabel.onTextChange = [this]() {
        String newText = textLabel.getText();
        setType(newText);
    };
}

void Box::mouseMove(const MouseEvent& e)
{
    findParentComponentOfClass<Canvas>()->repaint();
}

void Box::setType(String newType, bool exists)
{
    // Change box type
    textLabel.setText(newType, dontSendNotification);

    String arguments = newType.fromFirstOccurrenceOf(" ", false, false);
    String type = newType.upToFirstOccurrenceOf(" ", false, false);

    // Exists indicates that this object already exists in pd
    // When setting exists to true, you need to have assigned an object to the pdObject variable already
    if (!exists) {
        auto* pd = &cnv->patch;
        // Pd doesn't normally allow changing between gui and non-gui objects so we force it
        if (pdObject && graphics) {
            pd->removeObject(pdObject.get());
            pdObject = pd->createObject(newType, getX() - cnv->zeroPosition.x, getY() - cnv->zeroPosition.y);
        } else if (pdObject) {
            pdObject = pd->renameObject(pdObject.get(), newType);
        } else {
            pdObject = pd->createObject(newType, getX() - cnv->zeroPosition.x, getY() - cnv->zeroPosition.y);
        }
    }
    else {
        auto* ptr = pdObject->getPointer();
        // Reload GUI if it already exists
        if(pd::Gui::getType(ptr) != pd::Type::Undefined) {
            pdObject.reset(new pd::Gui(ptr, &cnv->patch, cnv->pd));
        }
        else {
            pdObject.reset(new pd::Object(ptr, &cnv->patch, cnv->pd));
        }
    }

    // Update inlets/outlets if it's not in a graph
    if (!cnv->isGraph)
        updatePorts();

    // Get best width for text
    auto bestWidth = std::max(45, textLabel.getFont().getStringWidth(newType) + 25);

    if (pdObject) {
        // Create graphics for the object if necessary
        graphics.reset(GUIComponent::createGui(type, this));

        if (graphics && graphics->getGUI().getType() == pd::Type::Comment) {
            setSize(bestWidth, 31);
        } else if (graphics && !graphics->fakeGUI()) {
            addAndMakeVisible(graphics.get());
            auto [w, h] = graphics->getBestSize();
            setSize(w + 8, h + 29);
            graphics->toBack();
            restrainer.checkComponentBounds(this);
        } else {
            setSize(bestWidth, 31);
        }
    } else {
        setSize(bestWidth, 31);
    }

    if (type.isEmpty()) {
        setSize(100, 31);
    }

    // Hide "comment" in front of name
    if (newType.startsWith("comment ")) {
        textLabel.setText(newType.fromFirstOccurrenceOf("comment ", false, false), dontSendNotification);
    }

    // IEM objects can be resized
    if (graphics && graphics->getGUI().isIEM()) {
        resizer.reset(new ResizableBorderComponent(this, &restrainer));
        addAndMakeVisible(resizer.get());
        resizer->toBack(); // make sure it's behind the edges
    } else {
        resizer.reset(nullptr);
    }

    cnv->main.updateUndoState();

    resized();
}

//==============================================================================
void Box::paint(Graphics& g)
{
    auto rect = getLocalBounds().reduced(4);

    auto baseColour = findColour(TextButton::buttonColourId);
    auto outlineColour = findColour(ComboBox::outlineColourId);

    bool isOver = getLocalBounds().contains(getMouseXYRelative());
    bool isDown = textLabel.isDown;

    bool selected = dragger.isSelected(this);

    bool hideLabel = graphics && !graphics->fakeGUI() && locked;

    if (isDown || isOver || selected) {
        baseColour = baseColour.contrasting(isDown ? 0.2f : 0.05f);
    }
    if (selected) {
        outlineColour = MainLook::highlightColour;
    }

    // Draw comment style
    if (graphics && graphics->getGUI().getType() == pd::Type::Comment) {
        g.setColour(outlineColour);
        g.drawRect(rect.toFloat(), 0.5f);
    }
    // Draw for all other objects
    else {
        if (!hideLabel) {
            g.setColour(baseColour);
            g.fillRoundedRectangle(rect.toFloat().withTrimmedBottom(getHeight() - 31), 2.0f);
        }

        g.setColour(outlineColour);
        g.drawRoundedRectangle(rect.toFloat(), 2.0f, 1.5f);
    }
}

void Box::moved()
{
    // Notify edges that we moved
    for (auto& edge : edges) {
        edge->sendMovedResizedMessages(true, true);
    }
}

void Box::resized()
{
    bool hideLabel = graphics && !graphics->fakeGUI() && locked;

    // Hidden header mode: gui objects become undraggable
    if (hideLabel) {
        graphics->setBounds(4, 4, getWidth() - 8, getHeight() - 8);
    } else {
        textLabel.setBounds(4, 4, getWidth() - 8, 22);
        auto bestWidth = textLabel.getFont().getStringWidth(textLabel.getText()) + 25;

        if (graphics && graphics->getGUI().getType() == pd::Type::Comment) {
            int num_lines = std::max(StringArray::fromTokens(textLabel.getText(), "\n", "\'").size(), 1);
            setSize(bestWidth + 30, (num_lines * 17) + 14);
            textLabel.setBounds(getLocalBounds().reduced(4));
        } else if (graphics) {
            graphics->setBounds(4, 25, getWidth() - 8, getHeight() - 29);
        }
    }

    if (resizer) {
        resizer->setBounds(getLocalBounds());
    }

    int index = 0;
    for (auto& edge : edges) {
        bool isInput = edge->isInput;
        int position = index < numInputs ? index : index - numInputs;
        int total = isInput ? numInputs : numOutputs;

        float newY = isInput ? 4 : getHeight() - 4;
        float newX = position * ((getWidth() - 32) / (total - 1 + (total == 1))) + 16;

        edge->setCentrePosition(newX, newY);
        edge->setSize(8, 8);

        index++;
    }
    
}

void Box::updatePorts()
{
    // update inlets and outlets

    int oldnumInputs = 0;
    int oldnumOutputs = 0;

    for (auto& edge : edges) {
        edge->isInput ? oldnumInputs++ : oldnumOutputs++;
    }

    numInputs = 0;
    numOutputs = 0;

    if (pdObject) {
        numInputs = pdObject->getNumInlets();
        numOutputs = pdObject->getNumOutlets();
    }

    // TODO: Make sure that this logic is exactly the same as PD
    while (numInputs < oldnumInputs) {
        edges.remove(oldnumInputs - 1);
        oldnumInputs--;
    }
    while (numInputs > oldnumInputs) {
        edges.insert(oldnumInputs, new Edge(this, true));
        oldnumInputs++;
    }

    while (numOutputs < oldnumOutputs) {
        edges.remove(oldnumOutputs - 1);
        oldnumOutputs--;
    }
    while (numOutputs > oldnumOutputs) {
        edges.insert(numInputs + oldnumOutputs, new Edge(this, false));
        oldnumOutputs++;
    }

    int numIn = 0;
    int numOut = 0;

    for (int i = 0; i < numInputs + numOutputs; i++) {
        auto* edge = edges[i];
        bool input = edge->isInput;
        bool isSignal = i < numInputs ? pdObject->isSignalInlet(i) : pdObject->isSignalOutlet(i - numInputs);

        edge->edgeIdx = input ? numIn : numOut;
        edge->isSignal = isSignal;

        numIn += input;
        numOut += !input;
    }

    resized();
}
