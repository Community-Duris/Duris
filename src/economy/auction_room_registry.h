#ifndef DURIS_AUCTION_ROOM_REGISTRY_H
#define DURIS_AUCTION_ROOM_REGISTRY_H

#include <cstddef>

// The collector and the auction house must agree on the exact service-room
// boundary. Keep the virtual room numbers behind this small immutable API so
// neither feature grows another hard-coded copy.
constexpr std::size_t AUCTION_HOUSE_REGISTERED_ROOM_COUNT = 10;

bool auction_house_registered_room_vnum(std::size_t index, int *room_vnum);
bool auction_house_is_registered_room_vnum(int room_vnum);

#endif
