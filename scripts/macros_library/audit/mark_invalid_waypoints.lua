-- Marks waypoint issues in console output.
local broken = 0
forEachWaypoint(function(name, x, y, z)
  if getTile(x, y, z) == nil then
    broken = broken + 1
    print("Broken waypoint: " .. name .. " at [" .. x .. "," .. y .. "," .. z .. "]")
  end
end)

print("invalid waypoint count=" .. tostring(broken))
