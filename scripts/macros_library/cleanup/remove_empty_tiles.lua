-- Removes tiles that contain no ground, item, creature or spawn.
-- Dry-run support: set DRY_RUN = true before running.

local removed = 0
forEachTile(function(tile)
  local hasGround = tile:ground() ~= nil
  local hasItems = tile:itemCount() > 0
  local hasSpawn = tile:hasSpawn()
  local hasCreature = tile:hasCreature()
  if not hasGround and not hasItems and not hasSpawn and not hasCreature then
    if not DRY_RUN then
      tile:delete()
    end
    removed = removed + 1
  end
end)

print("remove_empty_tiles removed=" .. tostring(removed) .. " dry_run=" .. tostring(DRY_RUN == true))
