#include "entity/drop.hpp"

void DropEntity::update(float dt)
{

}

ModelId DropEntity::get_model() const
{
    return item.get_model();
}

EntityType DropEntity::get_type() const
{
    return ENTITY_DROP;
}