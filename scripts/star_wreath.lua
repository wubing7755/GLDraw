local function add_star(cx, cy, radius)
    gldraw.document_add_object("fake_star", {
        x = cx - radius,
        y = cy - radius,
        width = radius * 2.0,
        height = radius * 2.0,
        stroke_width = 1.5
    })
end

function on_event(event)
    if event.name ~= "pointer_up" then
        return true
    end

    local star_count = 12
    local ring_radius = 80.0
    local star_radius = 18.0

    for i = 0, star_count - 1 do
        local angle = (math.pi * 2.0 * i) / star_count
        local x = event.x + math.cos(angle) * ring_radius
        local y = event.y + math.sin(angle) * ring_radius
        add_star(x, y, star_radius)
    end

    return true
end
