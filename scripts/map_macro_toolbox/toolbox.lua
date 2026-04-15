local function requireSelectionBounds()
    if not app or not app.map or not app.selection or app.selection.isEmpty then
        app.alert("Select one or more tiles before running macros.")
        return nil
    end
    return app.selection.bounds
end

local function clearIdsAndTextOnSelection()
    local bounds = requireSelectionBounds()
    if not bounds then
        return
    end

    local changedItems = 0
    app.transaction("Macro: Clear IDs/Text on Selection", function()
        for _, tile in ipairs(app.selection.tiles) do
            local function clearItemData(item)
                local touched = false
                if item.actionId ~= 0 then
                    item.actionId = 0
                    touched = true
                end
                if item.uniqueId ~= 0 then
                    item.uniqueId = 0
                    touched = true
                end
                if item.text and item.text ~= "" then
                    item.text = ""
                    touched = true
                end
                if touched then
                    changedItems = changedItems + 1
                end
            end

            if tile.ground then
                clearItemData(tile.ground)
            end

            for _, item in ipairs(tile.items) do
                clearItemData(item)
            end
        end
    end)

    app.alert(string.format("Clear IDs/Text complete.\nItems changed: %d", changedItems))
    app.refresh()
end

local function normalizeCreatureSpawnTime(targetSpawnTime)
    local bounds = requireSelectionBounds()
    if not bounds then
        return
    end

    local changedCreatures = 0
    app.transaction("Macro: Normalize Creature Spawn Time", function()
        for _, tile in ipairs(app.selection.tiles) do
            if tile.creature and tile.creature.spawnTime ~= targetSpawnTime then
                tile.creature.spawnTime = targetSpawnTime
                changedCreatures = changedCreatures + 1
            end
        end
    end)

    app.alert(string.format("Spawn time normalized to %d.\nCreatures changed: %d", targetSpawnTime, changedCreatures))
    app.refresh()
end

local function fillSelectionGround(groundItemId)
    local bounds = requireSelectionBounds()
    if not bounds then
        return
    end

    if not Items.exists(groundItemId) then
        app.alert(string.format("Ground item %d does not exist.", groundItemId))
        return
    end

    local info = Items.getInfo(groundItemId)
    if not info or not info.isGroundTile then
        app.alert(string.format("Item %d is not a ground item.", groundItemId))
        return
    end

    local changedTiles = 0
    app.transaction("Macro: Fill Selection Ground", function()
        for _, tile in ipairs(app.selection.tiles) do
            if not tile.ground or tile.ground.id ~= groundItemId then
                tile.ground = groundItemId
                changedTiles = changedTiles + 1
            end
        end
    end)

    app.alert(string.format("Ground fill complete.\nTiles changed: %d", changedTiles))
    app.refresh()
end

if not app then
    print("RME Lua API not available.")
    return
end

local dlg = Dialog {
    title = "Map Macro Toolbox",
    width = 420,
    height = 260,
    dockable = true,
    resizable = true
}

dlg:label {
    text = "Fast macros for selected tiles",
    font_weight = "bold"
}
dlg:newrow()
dlg:label {
    text = "Use on a tile selection to keep large maps consistent."
}
dlg:separator()

dlg:number {
    id = "ground_item_id",
    label = "Ground Item ID:",
    value = 4526,
    min = 1,
    max = 65535
}

dlg:number {
    id = "spawn_time",
    label = "Creature Spawn Time:",
    value = app.spawnTime or 60,
    min = 1,
    max = 3600
}

dlg:separator()

dlg:button {
    text = "Fill Selection Ground",
    onclick = function(d)
        fillSelectionGround(d.data.ground_item_id)
    end
}

dlg:button {
    text = "Clear Action/Unique/Text",
    onclick = function()
        clearIdsAndTextOnSelection()
    end
}

dlg:button {
    text = "Normalize Creature Spawn Time",
    onclick = function(d)
        normalizeCreatureSpawnTime(d.data.spawn_time)
    end
}

dlg:show { wait = false }
