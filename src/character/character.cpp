#include "character/character.h"

namespace mmo {
namespace character {

Character::Character()
    : character_id_(0)
    , player_id_(0)
    , name_("")
    , character_class_(CHARACTER_CLASS_WARRIOR)
    , level_(1)
    , experience_(0)
    , health_(100)
    , mana_(50)
    , strength_(10)
    , intelligence_(10)
    , agility_(10)
    , stamina_(10)
    , map_id_(1)
    , position_x_(0.0f)
    , position_y_(0.0f)
    , position_z_(0.0f)
    , created_time_(0)
    , last_login_time_(0) {
}

Character::~Character() {
}

} // namespace character
} // namespace mmo