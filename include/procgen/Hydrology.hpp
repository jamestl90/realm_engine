#pragma once

#include "GreaterRealm.hpp"

namespace procgen {

void build_greater_realm_drainage(GreaterRealmMap& map);
void build_greater_realm_river_channels(
    GreaterRealmMap& map,
    const GreaterRealmGeneratorSettings& settings
);

} // namespace procgen
