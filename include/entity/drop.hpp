#pragma once

#include "entity.hpp"
#include "item.hpp"

class DropEntity : public Entity
{
public:
    DropEntity(const Item& item = {})
        : item{item} {}

    void update(float dt) override;
    ModelId get_model() const override;
    EntityType get_type() const override;

private:
    Item item;
};