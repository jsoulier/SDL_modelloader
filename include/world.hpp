#pragma once

enum FloorId
{
    /* Don't change the order! */
    floor_overworld,
    floor_count,
};

bool init_world();

void shutdown_world();