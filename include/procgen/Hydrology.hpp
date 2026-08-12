#pragma once

#include "GreaterRealm.hpp"

namespace procgen {

void build_greater_realm_drainage(GreaterRealmMap& map);
void accumulate_greater_realm_rivers(
    GreaterRealmMap& map,
    const GreaterRealmGeneratorSettings& settings
);

} // namespace procgen
