#pragma once

#include "entity.hpp"
#include "item.hpp"

class DropEntity : public Entity
{
public:
    DropEntity(const Item& item = {})
        : item{item} {}

    void update(float dt) override;
    void render() const override;

private:
    Item item;
};