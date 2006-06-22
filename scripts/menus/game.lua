--            ____
--           / __ )____  _____
--          / __  / __ \/ ___/
--         / /_/ / /_/ (__  )
--        /_____/\____/____/
--
--      Invasion - Battle of Survival
--       A GPL'd futuristic RTS game
--
--      game.lua - In-game menus.
--
--      (c) Copyright 2006 by Jimmy Salmon
--
--      This program is free software; you can redistribute it and/or modify
--      it under the terms of the GNU General Public License as published by
--      the Free Software Foundation; only version 2 of the License.
--
--      This program is distributed in the hope that it will be useful,
--      but WITHOUT ANY WARRANTY; without even the implied warranty of
--      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--      GNU General Public License for more details.
--
--      You should have received a copy of the GNU General Public License
--      along with this program; if not, write to the Free Software
--      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
--      02111-1307, USA.
--
--      $Id$

function BosGameMenu()
  local menu = MenuScreen()
  menu:setSize(256, 288)
  menu:setPosition((Video.Width - menu:getWidth()) / 2,
    (Video.Height - menu:getHeight()) / 2)
  menu:setBorderSize(1)
  menu:setOpaque(true)
  menu:setBaseColor(dark)

  function menu:addLabel(text, x, y)
    local label = Label(text)
    label:setFont(Fonts["large"])
    label:adjustSize()
    self:add(label, x - label:getWidth() / 2, y)
  end

  function menu:addSmallButton(caption, x, y, callback)
    local b = ButtonWidget(caption)
    b:setActionCallback(callback)
    b:setSize(106, 28)
    b:setBackgroundColor(dark)
    b:setBaseColor(dark)
    menu:add(b, x, y)
  end

  function menu:addButton(caption, x, y, callback)
    local b = ButtonWidget(caption)
    b:setActionCallback(callback)
    b:setSize(224, 28)
    b:setBackgroundColor(dark)
    b:setBaseColor(dark)
    menu:add(b, x, y)
  end

  function menu:addCheckBox(caption, x, y, callback)
    local b = CheckBox(caption)
    b:setBaseColor(clear)
    b:setForegroundColor(clear)
    b:setBackgroundColor(dark)
    b:setActionCallback(function(s) callback(b, s) end)
    b:setFont(Fonts["game"])
    self:add(b, x, y)
    return b
  end

  function menu:addSlider(min, max, w, h, x, y, callback)
    local b = Slider(min, max)
    b:setBaseColor(dark)
    b:setForegroundColor(clear)
    b:setBackgroundColor(clear)
    b:setSize(w, h)
    b:setActionCallback(function(s) callback(b, s) end)
    self:add(b, x, y)
    return b
  end

  function menu:addListBox(x, y, w, h, list)
    local bq
    bq = ListBoxWidget(w, h)
    bq:setList(list)
    bq:setBaseColor(black)
    bq:setForegroundColor(clear)
    bq:setBackgroundColor(dark)
    bq:setFont(Fonts["game"])
    self:add(bq, x, y)   
    bq.itemslist = list
    return bq
  end

  function menu:addBrowser(path, filter, x, y, w, h, lister)
    local mapslist = {}
    local u = 1
    local fileslist
    local i
    local f
    if lister == nil then
       lister = ListFilesInDirectory
    end
    fileslist = lister(path)
    for i,f in fileslist do
      if(string.find(f, filter)) then
        mapslist[u] = f
        u = u + 1
      end
    end

    local bq
    bq = self:addListBox(x, y, w, h, mapslist)
    bq.getSelectedItem = function(self)
        if self:getSelected() < 0 then
           return self.itemslist[1]
        end
        return self.itemslist[self:getSelected() + 1]
    end

    return bq
  end

  function menu:addTextInputField(text, x, y, w)
    local b = TextField(text)
    b:setActionCallback(function() print("field") end)
    b:setFont(Fonts["game"])
    b:setBaseColor(clear)
    b:setForegroundColor(clear)
    b:setBackgroundColor(dark)
    b:setSize(w, 18)
    self:add(b, x, y)
    return b
  end

  return menu
end


function RunGameMenu(s)
  local menu = BosGameMenu()

  menu:addLabel(_("Game Menu"), 128, 11)
  -- FIXME: disable for multiplayer or replay
  menu:addSmallButton(_("Save (~<F11~>)"), 16, 40,
    function() RunSaveMenu() end)
  -- FIXME: disable for multiplayer
  menu:addSmallButton(_("Load (~<F12~>)"), 16 + 12 + 106, 40,
    function() RunLoadMenu() end)
  menu:addButton(_("Options (~<F5~>)"), 16, 40 + (36 * 1),
    function() RunGameOptionsMenu() end)
  menu:addButton(_("Help (~<F1~>)"), 16, 40 + (36 * 2),
    function() RunHelpMenu() end)
  menu:addButton(_("~!Objectives"), 16, 40 + (36 * 3),
    function() RunObjectivesMenu() end)
  menu:addButton(_("~!End Scenario"), 16, 40 + (36 * 4),
    function() RunEndScenarioMenu() end)
  menu:addButton(_("Return to Game (~<Esc~>)"), 16, 248,
    function() menu:stop(0) end)

  menu:run(false)
end

function RunSaveMenu()
  local menu = BosGameMenu()

  menu:addLabel(_("Save Game"), 128, 11)

  local t = menu:addTextInputField("game.sav", 16, 40, 224)

  local browser = menu:addBrowser("~save", ".sav.gz$", 16, 70, 224, 166)
  local function cb(s)
    t:setText(browser:getSelectedItem())
  end
  browser:setActionCallback(cb)

  menu:addSmallButton(_("Save"), 16, 248,
    function() SaveGame(t:getText()) menu:stop(0) end)
  menu:addSmallButton(_("Cancel"), 16 + 12 + 106, 248,
    function() menu:stop(0) end)

  menu:run(false)
end

function RunLoadMenu()
  local menu = BosGameMenu()

  menu:addLabel(_("Load Game"), 128, 11)

  local browser = menu:addBrowser("~save", ".sav.gz$", 16, 40, 224, 200)
  local function cb(s)
    print(browser:getSelectedItem())
  end
  browser:setActionCallback(cb)

  menu:addSmallButton(_("Load"), 16, 248,
    function() StartSavedGame("~save/"..browser:getSelectedItem()) end)

  menu:addSmallButton(_("Cancel"), 16 + 12 + 106, 248,
    function() menu:stop(0) end)

  menu:run(false)
end

function RunGameOptionsMenu()
  local menu = BosGameMenu()

  menu:addLabel(_("Game Options"), 128, 11)
  menu:addButton(_("Sound (~<F7~>)"), 16, 40 + (36 * 0),
    function() end)
  menu:addButton(_("Speeds (~<F8~>)"), 16, 40 + (36 * 1),
    function() RunGameSpeedOptionsMenu() end)
  menu:addButton(_("Preferences (~<F9~>)"), 16, 40 + (36 * 2),
    function() RunPreferencesMenu() end)
  menu:addButton(_("~!Diplomacy"), 16, 40 + (36 * 3),
    function() end)
  menu:addButton(_("Previous (~<Esc~>)"), 128 - (224 / 2), 248,
    function() menu:stop(0) end)

  menu:run(false)
end

function RunGameSpeedOptionsMenu()
  local menu = BosGameMenu()
  local l

  menu:addLabel(_("Speed Settings"), 128, 11)

  l = Label(_("Game Speed"))
  l:setFont(Fonts["game"])
  l:adjustSize()
  menu:add(l, 16, 36 * 1)

  local gamespeed = {}
  gamespeed = menu:addSlider(15, 75, 198, 18, 32, 36 * 1.5,
    function() SetGameSpeed(gamespeed:getValue()) end)
  gamespeed:setValue(GetGameSpeed())

  l = Label(_("slow"))
  l:setFont(Fonts["small"])
  l:adjustSize()
  menu:add(l, 34, (36 * 2) + 6)
  l = Label(_("fast"))
  l:setFont(Fonts["small"])
  l:adjustSize()
  menu:add(l, 230, (36 * 2) + 6)

  menu:addSmallButton(_("~!OK"), 128 - (106 / 2), 248,
    function() SavePreferences(); menu:stop(0) end)

  menu:run(false)
end

function RunPreferencesMenu()
  local menu = BosGameMenu()

  menu:addLabel(_("Preferences"), 128, 11)
  local fog = {}
  -- FIXME: disable checkbox for multiplayer or replays
  fog = menu:addCheckBox(_("Fog of War Enabled"), 16, 36 * 1,
    function() SetFogOfWar(fog:isMarked()) end)
  fog:setMarked(GetFogOfWar())
  local ckey = {}
  ckey = menu:addCheckBox(_("Show command key"), 16, 36 * 2,
    function() UI.ButtonPanel.ShowCommandKey = ckey:isMarked() end)
  ckey:setMarked(UI.ButtonPanel.ShowCommandKey)
  menu:addSmallButton(_("~!OK"), 128 - (106 / 2), 245,
    function() SavePreferences(); menu:stop(0) end)

  menu:run(false)
end

function RunEndScenarioMenu()
  local menu = BosGameMenu()

  menu:addLabel(_("End Scenario"), 128, 11)
  -- FIXME: disable for multiplayer
  menu:addButton(_("~!Restart Scenario"), 16, 40 + (36 * 0),
    function() RunRestartConfirmMenu() end)
  menu:addButton(_("~!Surrender"), 16, 40 + (36 * 1),
    function() RunSurrenderConfirmMenu() end)
  menu:addButton(_("~!Quit to Menu"), 16, 40 + (36 * 2),
    function() RunQuitToMenuConfirmMenu() end)
  menu:addButton(_("E~!xit Program"), 16, 40 + (36 * 3),
    function() RunExitConfirmMenu() end)
  menu:addButton(_("Previous (~<Esc~>)"), 16, 248,
    function() menu:stop(0) end)

  menu:run(false)
end

function RunRestartConfirmMenu()
  local menu = BosGameMenu()

  menu:addLabel(_("Are you sure you"), 128, 11)
  menu:addLabel(_("want to restart"), 128, 11 + (24 * 1))
  menu:addLabel(_("the scenario?"), 128, 11 + (24 * 2))
  menu:addButton(_("~!Restart Scenario"), 16, 11 + (24 * 3) + 29,
    function() end)
  menu:addButton(_("Cancel (~<Esc~>)"), 16, 248,
    function() menu:stop(0) end)

  menu:run(false)
end

function RunSurrenderConfirmMenu()
  local menu = BosGameMenu()

  menu:addLabel(_("Are you sure you"), 128, 11)
  menu:addLabel(_("want to surrender"), 128, 11 + (24 * 1))
  menu:addLabel(_("to your enemies?"), 128, 11 + (24 * 2))
  menu:addButton(_("~!Surrender"), 16, 11 + (24 * 3) + 29,
    function() end)
  menu:addButton(_("Cancel (~<Esc~>)"), 16, 248,
    function() menu:stop(0) end)

  menu:run(false)
end

function RunQuitToMenuConfirmMenu()
  local menu = BosGameMenu()

  menu:addLabel(_("Are you sure you"), 128, 11)
  menu:addLabel(_("want to quit to"), 128, 11 + (24 * 1))
  menu:addLabel(_("the main menu?"), 128, 11 + (24 * 2))
  menu:addButton(_("~!Quit to Menu"), 16, 11 + (24 * 3) + 29,
    function() end)
  menu:addButton(_("Cancel (~<Esc~>)"), 16, 248,
    function() menu:stop(0) end)

  menu:run(false)
end

function RunExitConfirmMenu()
  local menu = BosGameMenu()

  menu:addLabel(_("Are you sure you"), 128, 11)
  menu:addLabel(_("want to exit"), 128, 11 + (24 * 1))
  menu:addLabel(_("Stratagus?"), 128, 11 + (24 * 2))
  menu:addButton(_("E~!xit Program"), 16, 11 + (24 * 3) + 29,
    function() end)
  menu:addButton(_("Cancel (~<Esc~>)"), 16, 248,
    function() menu:stop(0) end)

  menu:run(false)
end

function RunHelpMenu()
  local menu = BosGameMenu()

  menu:addLabel(_("Help Menu"), 128, 11)
  menu:addButton(_("Keystroke ~!Help"), 16, 40 + (36 * 0),
    function() RunKeystrokeHelpMenu() end)
  menu:addButton(_("Stratagus ~!Tips"), 16, 40 + (36 * 1),
    function() RunTipsMenu() end)
  menu:addButton(_("Previous (~<Esc~>)"), 128 - (224 / 2), 248,
    function() menu:stop(0) end)

  menu:run(false)
end

local keystrokes = {
  {"Alt-F", "- toggle full screen"},
  {"Alt-G", "- toggle grab mouse"},
  {"Ctrl-S", "- mute sound"},
  {"Ctrl-M", "- mute music"},
  {"+", "- increase game speed"},
  {"-", "- decrease game speed"},
  {"Ctrl-P", "- pause game"},
  {"PAUSE", "- pause game"},
  {"PRINT", "- make screen shot"},
  {"Alt-H", "- help menu"},
  {"Alt-R", "- restart scenario"},
  {"Alt-Q", "- quit to main menu"},
  {"Alt-X", "- quit game"},
  {"Alt-B", "- toggle expand map"},
  {"Alt-M", "- game menu"},
  {"ENTER", "- write a message"},
  {"SPACE", "- goto last event"},
  {"TAB", "- hide/unhide terrain"},
  {"Ctrl-T", "- track unit"},
  {"Alt-I", "- find idle peon"},
  {"Alt-C", "- center on selected unit"},
  {"Alt-V", "- next view port"},
  {"Ctrl-V", "- previous view port"},
  {"^", "- select nothing"},
  {"#", "- select group"},
  {"##", "- center on group"},
  {"Ctrl-#", "- define group"},
  {"Shift-#", "- add to group"},
  {"Alt-#", "- add to alternate group"},
  {"F2-F4", "- recall map position"},
  {"Shift F2-F4", "- save map postition"},
  {"F5", "- game options"},
  {"F7", "- sound options"},
  {"F8", "- speed options"},
  {"F9", "- preferences"},
  {"F10", "- game menu"},
  {"F11", "- save game"},
  {"F12", "- load game"},
}

function RunKeystrokeHelpMenu()
  local menu = BosGameMenu()
  menu:setSize(352, 352)
  menu:setPosition((Video.Width - menu:getWidth()) / 2,
    (Video.Height - menu:getHeight()) / 2)

  local c = Container()
  c:setOpaque(false)

  for i=1,table.getn(keystrokes) do
    local l = Label(keystrokes[i][1])
    l:setFont(Fonts["game"])
    l:adjustSize()
    c:add(l, 0, 20 * (i - 1))
    local l = Label(keystrokes[i][2])
    l:setFont(Fonts["game"])
    l:adjustSize()
    c:add(l, 80, 20 * (i - 1))
  end

  local s = ScrollArea()
  c:setSize(320 - s:getScrollbarWidth(), 20 * table.getn(keystrokes))
  s:setBaseColor(dark)
  s:setBackgroundColor(dark)
  s:setForegroundColor(clear)
  s:setSize(320, 216)
  s:setContent(c)
  menu:add(s, 16, 60)

  menu:addLabel(_("Keystroke Help Menu"), 352 / 2, 11)
  menu:addButton(_("Previous (~<Esc~>)"), (352 / 2) - (224 / 2), 352 - 40,
    function() menu:stop(0) end)

  menu:run(false)
end

local tips = {
  "You can select all of your currently visible units of the same type by holding down the CTRL key and selecting a unit or by \"double clicking\" on a unit.",
  "The more engineers you have collecting resources, the faster your economy will grow.",

  "Use your engineers to repair damaged buildings.",
  "Explore your surroundings early in the game.",


  "Keep all engineers working. Use ALT-I to find idle engineers.",
  "You can make units automatically cast spells by selecting a unit, holding down CTRL and clicking on the spell icon.  CTRL click again to turn off.",

  -- Shift tips
  "You can give an unit an order which is executed after it finishes the current work, if you hold the SHIFT key.",
  "You can give way points, if you press the SHIFT key, while you click right for the move command.",
  "You can order an engineer to build one building after the other, if you hold the SHIFT key, while you place the building.",
  "You can build many of the same building, if you hold the ALT and SHIFT keys while you place the buildings.",

  "Use CTRL-V or ALT-V to cycle through the viewport configuration, you can than monitor your base and lead an attack.",

  "Know a useful tip?  Then add it here!",
}
local tipnumber = 0

function RunTipsMenu()
  local menu = BosGameMenu()
  menu:setSize(384, 256)
  menu:setPosition((Video.Width - menu:getWidth()) / 2,
    (Video.Height - menu:getHeight()) / 2)

  menu:addLabel(_("Stratagus Tips"), 192, 11)

  local l = MultiLineLabel()
  l:setFont(Fonts["game"])
  l:setSize(356, 144)
  l:setLineWidth(356)
  menu:add(l, 14, 36)
  -- FIXME: save tipnumber
  function l:prevTip()
    tipnumber = tipnumber - 1
    if (tipnumber < 1) then
      tipnumber = table.getn(tips)
    end
    self:setCaption(tips[tipnumber])
  end
  function l:nextTip()
    tipnumber = tipnumber + 1
    if (tipnumber > table.getn(tips)) then
      tipnumber = 1
    end
    self:setCaption(tips[tipnumber])
  end
  if (tipnumber == 0) then
    l:nextTip()
  end

  --FIXME: save setting
  menu:addCheckBox(_("Show tips at startup"), 14, 256 - 75,
    function() end)
  menu:addSmallButton(_("~!OK"), 14, 256 - 40,
    function() menu:stop(0) end)
  menu:addSmallButton(_("~!Previous Tip"), 14 + 106 + 11, 256 - 40,
    function() l:prevTip() end)
  menu:addSmallButton(_("~!Next Tip"), 14 + 106 + 11 + 106 + 11, 256 - 40,
    function() l:nextTip() end)


  menu:run(false)
end

function RunObjectivesMenu()
  local menu = BosGameMenu()
  menu:setSize(384, 256)
  menu:setPosition((Video.Width - menu:getWidth()) / 2,
    (Video.Height - menu:getHeight()) / 2)

  menu:addLabel(_("Objectives"), 192, 11)

  local l = MultiLineLabel()
  l:setFont(Fonts["game"])
  l:setSize(356, 144)
  l:setLineWidth(356)
  menu:add(l, 14, 36)

  --FIXME: l:setCaption()

  menu:addButton(_("~!OK"), 80, 256 - 40,
    function() menu:stop(0) end)

  menu:run(false)
end

function RunVictoryMenu()
  local menu = BosGameMenu()
  menu:setSize(288, 128)
  menu:setPosition((Video.Width - menu:getWidth()) / 2,
    (Video.Height - menu:getHeight()) / 2)

  menu:addLabel(_("Congratulations!"), 144, 11)
  menu:addLabel(_("You are victorious!"), 144, 32)
  menu:addButton(_("~!Victory"), 32, 54,
    function() menu:stop(0) end)
  -- FIXME: check if log is disabled
  menu:addButton(_("Save ~!Replay"), 32, 90,
    function() RunSaveReplayMenu() end)

  menu:run(false)
end

function RunDefeatMenu()
  local menu = BosGameMenu()
  menu:setSize(288, 128)
  menu:setPosition((Video.Width - menu:getWidth()) / 2,
    (Video.Height - menu:getHeight()) / 2)

  menu:addLabel(_("You have failed to"), 144, 11)
  menu:addLabel(_("achieve victory!"), 144, 32)
  menu:addButton(_("~!OK"), 32, 56,
    function() menu:stop(0) end)
  -- FIXME: check if log is disabled
  menu:addButton(_("Save ~!Replay"), 32, 90,
    function() RunSaveReplayMenu() end)

  menu:run(false)
end

function RunSaveReplayMenu()
  local menu = BosGameMenu()
  menu:setSize(288, 128)
  menu:setPosition((Video.Width - menu:getWidth()) / 2,
    (Video.Height - menu:getHeight()) / 2)

  menu:addLabel(_("Save Replay"), 144, 11)
  menu:addTextInputField("", 14, 40, 260)
  menu:addSmallButton(_("~!OK"), 14, 80,
    function() menu:stop(0) end)
  menu:addSmallButton(_("Cancel (~<Esc~>)"), 162, 80,
    function() menu:stop(0) end)

  menu:run(false)
end

