#include "games/eldenring_data.h"
#include <algorithm>
#include <cstdio>
#include <set>

namespace {

struct MapEntry { uint32_t id; const char* name; };

// Base game names come from the Smithbox map id list (MIT, github.com/vawser/Smithbox,
// "Documentation/Game Information/ER/Map/Info - Map IDs.txt"). Realm of Shadow tiles carry only
// their region, from the in-game place names. Sorted by id.
const MapEntry kMaps[] = {
    { 0x0A000000, "Stormveil Castle" },   // m10_00_00_00
    { 0x0A010000, "Chapel of Anticipation" },   // m10_01_00_00
    { 0x0B000000, "Leyndell, Royal Capital" },   // m11_00_00_00
    { 0x0B050000, "Leyndell, Ashen Capital" },   // m11_05_00_00
    { 0x0B0A0000, "Roundtable Hold" },   // m11_10_00_00
    { 0x0C010000, "Ainsel River" },   // m12_01_00_00
    { 0x0C020000, "Siofra River" },   // m12_02_00_00
    { 0x0C030000, "Deeproot Depths" },   // m12_03_00_00
    { 0x0C040000, "Ainsel River - Boss" },   // m12_04_00_00
    { 0x0C050000, "Mohgwyn Palace" },   // m12_05_00_00
    { 0x0C070000, "Siofra River Bank" },   // m12_07_00_00
    { 0x0C080000, "Siofra River - Boss" },   // m12_08_00_00
    { 0x0C090000, "Nokron, Eternal City - Boss" },   // m12_09_00_00
    { 0x0D000000, "Crumbling Farum Azula" },   // m13_00_00_00
    { 0x0E000000, "Academy of Raya Lucaria" },   // m14_00_00_00
    { 0x0F000000, "Miquella's Haligtree" },   // m15_00_00_00
    { 0x10000000, "Volcano Manor" },   // m16_00_00_00
    { 0x12000000, "Stranded Graveyard" },   // m18_00_00_00
    { 0x13000000, "Stone Platform" },   // m19_00_00_00
    { 0x14000000, "Belurat, Tower Settlement" },   // m20_00_00_00
    { 0x14010000, "Enir-Ilim" },   // m20_01_00_00
    { 0x15000000, "Shadow Keep" },   // m21_00_00_00
    { 0x15010000, "Specimen Storehouse" },   // m21_01_00_00
    { 0x15020000, "Shadow Keep - West Rampart" },   // m21_02_00_00
    { 0x16000000, "Stone Coffin Fissure" },   // m22_00_00_00
    { 0x19000000, "Finger Birthing Grounds" },   // m25_00_00_00
    { 0x1C000000, "Midra's Manse" },   // m28_00_00_00
    { 0x1E000000, "Tombsward Catacombs" },   // m30_00_00_00
    { 0x1E010000, "Impaler's Catacombs" },   // m30_01_00_00
    { 0x1E020000, "Stormfoot Catacombs" },   // m30_02_00_00
    { 0x1E030000, "Road's End Catacombs" },   // m30_03_00_00
    { 0x1E040000, "Murkwater Catacombs" },   // m30_04_00_00
    { 0x1E050000, "Black Knife Catacombs" },   // m30_05_00_00
    { 0x1E060000, "Cliffbottom Catacombs" },   // m30_06_00_00
    { 0x1E070000, "Wyndham Catacombs" },   // m30_07_00_00
    { 0x1E080000, "Sainted Hero's Grave" },   // m30_08_00_00
    { 0x1E090000, "Gelmir Hero's Grave" },   // m30_09_00_00
    { 0x1E0A0000, "Auriza Hero's Grave" },   // m30_10_00_00
    { 0x1E0B0000, "Deathtouched Catacombs" },   // m30_11_00_00
    { 0x1E0C0000, "Unsightly Catacombs" },   // m30_12_00_00
    { 0x1E0D0000, "Auriza Side Tomb" },   // m30_13_00_00
    { 0x1E0E0000, "Minor Erdtree Catacombs" },   // m30_14_00_00
    { 0x1E0F0000, "Caelid Catacombs" },   // m30_15_00_00
    { 0x1E100000, "War-Dead Catacombs" },   // m30_16_00_00
    { 0x1E110000, "Giant-Conquering Hero's Grave" },   // m30_17_00_00
    { 0x1E120000, "Giants' Mountaintop Catacombs" },   // m30_18_00_00
    { 0x1E130000, "Consecrated Snowfield Catacombs" },   // m30_19_00_00
    { 0x1E140000, "Hidden Path to the Haligtree" },   // m30_20_00_00
    { 0x1F000000, "Murkwater Cave" },   // m31_00_00_00
    { 0x1F010000, "Earthbore Cave" },   // m31_01_00_00
    { 0x1F020000, "Tombsward Cave" },   // m31_02_00_00
    { 0x1F030000, "Groveside Cave" },   // m31_03_00_00
    { 0x1F040000, "Stillwater Cave" },   // m31_04_00_00
    { 0x1F050000, "Lakeside Crystal Cave" },   // m31_05_00_00
    { 0x1F060000, "Academy Crystal Cave" },   // m31_06_00_00
    { 0x1F070000, "Seethewater Cave" },   // m31_07_00_00
    { 0x1F090000, "Volcano Cave" },   // m31_09_00_00
    { 0x1F0A0000, "Dragonbarrow Cave" },   // m31_10_00_00
    { 0x1F0B0000, "Sellia Hideaway" },   // m31_11_00_00
    { 0x1F0C0000, "Cave of the Forlorn" },   // m31_12_00_00
    { 0x1F0F0000, "Coastal Cave" },   // m31_15_00_00
    { 0x1F110000, "Highroad Cave" },   // m31_17_00_00
    { 0x1F120000, "Perfumer's Grotto" },   // m31_18_00_00
    { 0x1F130000, "Sage's Cave" },   // m31_19_00_00
    { 0x1F140000, "Abandoned Cave" },   // m31_20_00_00
    { 0x1F150000, "Gaol Cave" },   // m31_21_00_00
    { 0x1F160000, "Spiritcaller's Cave" },   // m31_22_00_00
    { 0x20000000, "Morne Tunnel" },   // m32_00_00_00
    { 0x20010000, "Limgrave Tunnels" },   // m32_01_00_00
    { 0x20020000, "Raya Lucaria Crystal Tunnel" },   // m32_02_00_00
    { 0x20040000, "Old Altus Tunnel" },   // m32_04_00_00
    { 0x20050000, "Altus Tunnel" },   // m32_05_00_00
    { 0x20070000, "Gael Tunnel" },   // m32_07_00_00
    { 0x20080000, "Sellia Crystal Tunnel" },   // m32_08_00_00
    { 0x200B0000, "Yelough Anix Tunnel" },   // m32_11_00_00
    { 0x220A0000, "Divine Tower of Limgrave" },   // m34_10_00_00
    { 0x220B0000, "Divine Tower of Liurnia" },   // m34_11_00_00
    { 0x220C0000, "Divine Tower of West Altus" },   // m34_12_00_00
    { 0x220D0000, "Divine Tower of Caelid" },   // m34_13_00_00
    { 0x220E0000, "Divine Tower of East Altus" },   // m34_14_00_00
    { 0x220F0000, "Isolated Divine Tower" },   // m34_15_00_00
    { 0x23000000, "Subterranean Shunning-Grounds" },   // m35_00_00_00
    { 0x27140000, "Ruin-Strewn Precipice" },   // m39_20_00_00
    { 0x28000000, "Fog Rift Catacombs" },   // m40_00_00_00
    { 0x28010000, "Scorpion River Catacombs" },   // m40_01_00_00
    { 0x28020000, "Darklight Catacombs" },   // m40_02_00_00
    { 0x29000000, "Belurat Gaol" },   // m41_00_00_00
    { 0x29010000, "Bonny Gaol" },   // m41_01_00_00
    { 0x29020000, "Lamenter's Gaol" },   // m41_02_00_00
    { 0x2A000000, "Ruined Forge Lava Intake" },   // m42_00_00_00
    { 0x2A020000, "Ruined Forge of Starfall Past" },   // m42_02_00_00
    { 0x2A030000, "Taylew's Ruined Forge" },   // m42_03_00_00
    { 0x2B000000, "Rivermouth Cave" },   // m43_00_00_00
    { 0x2B010000, "Dragon's Pit" },   // m43_01_00_00
    { 0x2D000000, "Royal Colosseum" },   // m45_00_00_00
    { 0x2D010000, "Caelid Colosseum" },   // m45_01_00_00
    { 0x2D020000, "Limgrave Colosseum" },   // m45_02_00_00
    { 0x3C212800, "Moonlight Altar - Altar South, Chelona's Rise" },   // m60_33_40_00
    { 0x3C212900, "Moonlight Altar - West of Deep Ainsel Well" },   // m60_33_41_00
    { 0x3C212A00, "Moonlight Altar - Ringleader's Evergaol" },   // m60_33_42_00
    { 0x3C212B00, "Liurnia of the Lakes - Minor Erdtree, Road's End Catacombs Entrance" },   // m60_33_43_00
    { 0x3C212C00, "Liurnia of the Lakes - Revenger's Shack" },   // m60_33_44_00
    { 0x3C212D00, "Liurnia of the Lakes - Cuckoo's Evergaol, West of Meeting Place" },   // m60_33_45_00
    { 0x3C212E00, "Liurnia of the Lakes - Foot of the Four Belfries" },   // m60_33_46_00
    { 0x3C212F00, "Liurnia of the Lakes - The Four Belfries, Giant's Caravan Below" },   // m60_33_47_00
    { 0x3C222900, "Moonlight Altar - Deep Ainsel Well" },   // m60_34_41_00
    { 0x3C222A00, "Liurnia of the Lakes - Village of the Albinaurics; Moonlight Altar - Moonfolk Ruins" },   // m60_34_42_00
    { 0x3C222B00, "Liurnia of the Lakes - Converted Tower" },   // m60_34_43_00
    { 0x3C222C00, "Liurnia of the Lakes - Temple Quarter" },   // m60_34_44_00
    { 0x3C222D00, "Liurnia of the Lakes - Meeting Place, Academy Crystal Cave Entrance" },   // m60_34_45_00
    { 0x3C222E00, "Liurnia of the Lakes - Crystalline Woods, Path to the Four Belfries" },   // m60_34_46_00
    { 0x3C222F00, "Liurnia of the Lakes - Sorcerer's Isle West, East Belfry" },   // m60_34_47_00
    { 0x3C223000, "Liurnia of the Lakes - Northern Liurnia Lake Shore, Kingsrealm Ruins" },   // m60_34_48_00
    { 0x3C223100, "Liurnia of the Lakes - Road to the Manor" },   // m60_34_49_00
    { 0x3C223200, "Liurnia of the Lakes - Ranni's Rise, Seluvis's Rise" },   // m60_34_50_00
    { 0x3C223300, "Liurnia of the Lakes - Renna's Rise" },   // m60_34_51_00
    { 0x3C232900, "Moonlight Altar - Lunar Estate Ruins" },   // m60_35_41_00
    { 0x3C232A00, "Moonlight Altar - Cathedral of Manus Celes" },   // m60_35_42_00
    { 0x3C232B00, "Liurnia of the Lakes - Folly on the Lake, South Rose Church" },   // m60_35_43_00
    { 0x3C232C00, "Liurnia of the Lakes - Far West Gate Town, North Rose Church" },   // m60_35_44_00
    { 0x3C232D00, "Liurnia of the Lakes - South Raya Lucaria Gate" },   // m60_35_45_00
    { 0x3C232E00, "Liurnia of the Lakes - Main Academy Gate, Academy of Raya Lucaria Entrance" },   // m60_35_46_00
    { 0x3C232F00, "Liurnia of the Lakes - Testu's Rise, East Gate Bridge Trestle" },   // m60_35_47_00
    { 0x3C233000, "Liurnia of the Lakes - North of Sorcerer's Isle" },   // m60_35_48_00
    { 0x3C233100, "Liurnia of the Lakes - South of Caria Manor, West of Ravine Entrance" },   // m60_35_49_00
    { 0x3C233200, "Liurnia of the Lakes - Caria Manor" },   // m60_35_50_00
    { 0x3C233300, "Liurnia of the Lakes - North of Caria Manor" },   // m60_35_51_00
    { 0x3C233400, "Mt. Gelmir - Before Hermit Shack" },   // m60_35_52_00
    { 0x3C233500, "Mt. Gelmir - Seethewater Terminus" },   // m60_35_53_00
    { 0x3C233600, "Mt. Gelmir - Fort Laiedd" },   // m60_35_54_00
    { 0x3C242900, "Liurnia of the Lakes - Slumbering Wolf's Shack, Lakeside Crystal Cave Entrance" },   // m60_36_41_00
    { 0x3C242A00, "Liurnia of the Lakes - West of Scenic Isle" },   // m60_36_42_00
    { 0x3C242B00, "Liurnia of the Lakes - Boilprawn Shack, Fallen Ruins of the Lake" },   // m60_36_43_00
    { 0x3C242C00, "Liurnia of the Lakes - Gate Town Southwest" },   // m60_36_44_00
    { 0x3C242D00, "Liurnia of the Lakes - Gate Town Northwest" },   // m60_36_45_00
    { 0x3C242E00, "Liurnia of the Lakes - West of Church of Vows" },   // m60_36_46_00
    { 0x3C242F00, "Liurnia of the Lakes - West of Raya Lucaria Crystal Tunnel; Bellum Highway - South of East Raya Lucaria Gate" },   // m60_36_47_00
    { 0x3C243000, "Bellum Highway - East Raya Lucaria Gate" },   // m60_36_48_00
    { 0x3C243100, "Liurnia of the Lakes - Southeast Ravine, Lake at Caria Manor Entrance; Bellum Highway - Bellum Church" },   // m60_36_49_00
    { 0x3C243200, "Liurnia of the Lakes - Behind Caria Manor, Royal Grave Evergaol, Part of Ravine" },   // m60_36_50_00
    { 0x3C243300, "Altus Plateau - Perfumer's Ruins, Unsightly Catacombs Entrance" },   // m60_36_51_00
    { 0x3C243400, "Mt. Gelmir - Craftsman's Shack, Hermit's Shack" },   // m60_36_52_00
    { 0x3C243500, "Mt. Gelmir - Outside Volcano Manor" },   // m60_36_53_00
    { 0x3C243600, "Mt. Gelmir - Ninth Mt. Gelmir Campsite, Road of Iniquity, Volcano Manor Entrance" },   // m60_36_54_00
    { 0x3C252900, "Liurnia of the Lakes - South of Scenic Isle, East of Slumbering Wolf's Shack" },   // m60_37_41_00
    { 0x3C252A00, "Liurnia of the Lakes - Scenic Isle, Laskyar Ruins" },   // m60_37_42_00
    { 0x3C252B00, "Liurnia of the Lakes - North of Scenic Isle" },   // m60_37_43_00
    { 0x3C252C00, "Liurnia of the Lakes - Gate Town Southeast" },   // m60_37_44_00
    { 0x3C252D00, "Liurnia of the Lakes - Gate Town Northeast" },   // m60_37_45_00
    { 0x3C252E00, "Liurnia of the Lakes - Church of Vows" },   // m60_37_46_00
    { 0x3C252F00, "Liurnia of the Lakes - South of Mausoleum Compound, Raya Lucaria Crystal Tunnel Entrance" },   // m60_37_47_00
    { 0x3C253000, "Liurnia of the Lakes - Mausoleum Compound" },   // m60_37_48_00
    { 0x3C253100, "Bellum Highway - Church of Inhibition, Southwest of Grand Lift of Dectus" },   // m60_37_49_00
    { 0x3C253200, "Liurnia of the Lake - Northeast Ravine; Bellum Highway - West of Grand Lift of Dectus" },   // m60_37_50_00
    { 0x3C253300, "Altus Plateau - Abandoned Coffin, Ruin-Strewn Precipice Exit" },   // m60_37_51_00
    { 0x3C253400, "Mt. Gelmir - Seethewater River, Hermit Village, Wyndham Catacombs Entrance, Sage's Cave Entrance" },   // m60_37_52_00
    { 0x3C253500, "Mt. Gelmir - Primeval Sorcerer Azur, Gelmir Hero's Grave Entrance" },   // m60_37_53_00
    { 0x3C253600, "Mt. Gelmir - Minor Erdtree, Seethewater Cave Entrance" },   // m60_37_54_00
    { 0x3C253700, "Mt. Gelmir - Volcano Cave Entrance" },   // m60_37_55_00
    { 0x3C262700, "Liurnia of the Lakes - West of Church of Irith" },   // m60_38_39_00
    { 0x3C262800, "Liurnia of the Lakes - Liurnia Lake Shore" },   // m60_38_40_00
    { 0x3C262900, "Liurnia of the Lakes - Laskyar Ruins, Malefactor's Evergaol" },   // m60_38_41_00
    { 0x3C262A00, "Liurnia of the Lakes - Highway Lookout Tower" },   // m60_38_42_00
    { 0x3C262B00, "Liurnia of the Lakes - Gate Town Bridge" },   // m60_38_43_00
    { 0x3C262C00, "Liurnia of the Lakes - North of Gate Town Bridge" },   // m60_38_44_00
    { 0x3C262D00, "Liurnia of the Lakes - Artist's Shack, Eastern Liurnia Lake Shore" },   // m60_38_45_00
    { 0x3C262E00, "Liurnia of the Lakes - Eastern Tableland, Ainsel River Well" },   // m60_38_46_00
    { 0x3C262F00, "Liurnia of the Lakes - Ruined Labyrinth, Uld Palace Ruins" },   // m60_38_47_00
    { 0x3C263000, "Bellum Highway - Frenzied Flame Village, Minor Erdtree" },   // m60_38_48_00
    { 0x3C263100, "Bellum Highway - Frenzy-Flaming Tower" },   // m60_38_49_00
    { 0x3C263200, "Liurnia of the Lakes - Ravine-Veiled Village; Bellum Highway - Grand Lift of Dectus; Altus Plateau - Altus Plateau; Ruin-Strewn Precipice Entrance" },   // m60_38_50_00
    { 0x3C263300, "Altus Plateau - Erdtree-Gazing Hill, Lux Ruins" },   // m60_38_51_00
    { 0x3C263400, "Altus Plateau - Wyndham Ruins, Wyndham Catacombs Entrance" },   // m60_38_52_00
    { 0x3C263500, "Altus Plateau - Old Altus Tunnel Entrance" },   // m60_38_53_00
    { 0x3C263600, "Mt. Gelmir - First Mt. Gelmir Campsite, Corpse-Stench Shack; Altus Plateau - West of Shaded Castle" },   // m60_38_54_00
    { 0x3C272700, "Liurnia of the Lakes - Church of Irith" },   // m60_39_39_00
    { 0x3C272800, "Liurnia of the Lakes - Lake-Facing Cliffs, Stormveil Castle Exit, Stillwater Cave Entrance" },   // m60_39_40_00
    { 0x3C272900, "Liurnia of the Lakes - Liurnia Highway South, Purified Ruins, Cliffbottom Catacombs Entrance" },   // m60_39_41_00
    { 0x3C272A00, "Liurnia of the Lakes - Liurnia Highway North" },   // m60_39_42_00
    { 0x3C272B00, "Liurnia of the Lakes - Liurnia Highway Far North" },   // m60_39_43_00
    { 0x3C272C00, "Liurnia of the Lakes - Jarburg, Divine Tower of Liurnia Entrance" },   // m60_39_44_00
    { 0x3C272D00, "Liurnia of the Lakes - Eastern Tableland Southeast Cliffside" },   // m60_39_45_00
    { 0x3C272E00, "Liurnia of the Lakes - Eastern Tableland Northeast Cliffside" },   // m60_39_46_00
    { 0x3C273000, "Bellum Highway - Converted Fringe Tower; Liurnia of the Lakes - Black Knife Catacombs Entrance" },   // m60_39_48_00
    { 0x3C273100, "Bellum Highway - Converted Fringe Tower North Cliffside" },   // m60_39_49_00
    { 0x3C273200, "Altus Plateau - Golden Lineage Evergaol" },   // m60_39_50_00
    { 0x3C273300, "Altus Plateau - Altus Highway Junction" },   // m60_39_51_00
    { 0x3C273400, "Altus Plateau - Second Church of Marika" },   // m60_39_52_00
    { 0x3C273500, "Altus Plateau - Mirage Rise; Mt. Gelmir - Bridge of Iniquity" },   // m60_39_53_00
    { 0x3C273600, "Altus Plateau - Shaded Castle" },   // m60_39_54_00
    { 0x3C282100, "Weeping Peninsula - Far West Shore" },   // m60_40_33_00
    { 0x3C282600, "Stormhill - Forested West Cliffside" },   // m60_40_38_00
    { 0x3C282700, "Stormhill - Path to Liurnia" },   // m60_40_39_00
    { 0x3C282800, "Liurnia - Liurnia Highway South Endpoint, Path to Stormhill" },   // m60_40_40_00
    { 0x3C283200, "Altus Plateau - Southwest of Tree Sentinel Duo" },   // m60_40_50_00
    { 0x3C283300, "Altus Plateau - West of Tree Sentinel Duo, Stormcaller Church" },   // m60_40_51_00
    { 0x3C283400, "Altus Plateau - Forest-Spanning Greatbridge, St. Trina's Hideaway, Sainted Hero's Grave Entrance" },   // m60_40_52_00
    { 0x3C283500, "Altus Plateau - Bower of Bounty, Writheblood Ruins" },   // m60_40_53_00
    { 0x3C283600, "Altus Plateau - Road of Iniquity Side Path" },   // m60_40_54_00
    { 0x3C283700, "Altus Plateau - West Windmill Pasture" },   // m60_40_55_00
    { 0x3C292000, "Weeping Peninsula - Isolated Merchant's Shack" },   // m60_41_32_00
    { 0x3C292100, "Weeping Peninsula - Fourth Church of Marika, Witchbane Ruins" },   // m60_41_33_00
    { 0x3C292200, "Weeping Peninsula - North of Fourth Church of Marika" },   // m60_41_34_00
    { 0x3C292300, "Limgrave - Church of Dragon Communion, Coastal Cave Exit" },   // m60_41_35_00
    { 0x3C292400, "Limgrave - Coastal Cave Entrance" },   // m60_41_36_00
    { 0x3C292500, "Limgrave - Stormfoot Catacombs Entrance" },   // m60_41_37_00
    { 0x3C292600, "Stormhill - Stormhill Shack, Stormveil Castle Entrance" },   // m60_41_38_00
    { 0x3C292700, "Stormhill - North of Stormhill Shack" },   // m60_41_39_00
    { 0x3C293200, "Altus Plateau - South of Tree Sentinel Duo" },   // m60_41_50_00
    { 0x3C293300, "Altus Plateau - Tree Sentinel Duo, Perfumer's Grotto Entrance" },   // m60_41_51_00
    { 0x3C293400, "Altus Plateau - Rampartside Path, Altus Tunnel Entrance" },   // m60_41_52_00
    { 0x3C293500, "Altus Plateau - Woodfolk Ruins, Minor Erdtree" },   // m60_41_53_00
    { 0x3C293600, "Altus Plateau - West Windmill Village" },   // m60_41_54_00
    { 0x3C293700, "Altus Plateau - Village Windmill Pasture, East Windmill Pasture" },   // m60_41_55_00
    { 0x3C2A2000, "Weeping Peninsula - Tower of Return" },   // m60_42_32_00
    { 0x3C2A2100, "Weeping Peninsula - Tombsward; Weeping Evergaol; Tombsward Cave Entrance" },   // m60_42_33_00
    { 0x3C2A2200, "Weeping Peninsula - Tombsward Ruins" },   // m60_42_34_00
    { 0x3C2A2300, "Limgrave - South of Stranded Graveyard Exit" },   // m60_42_35_00
    { 0x3C2A2400, "Limgrave - Church of Elleh, Stranded Graveyard Exit" },   // m60_42_36_00
    { 0x3C2A2500, "Limgrave - Gatefront, Stormhill Evergaol, Groveside Cave Entrance, Limgrave Tunnels Entrance" },   // m60_42_37_00
    { 0x3C2A2600, "Stormhill - Warmaster's Shack, Stormgate" },   // m60_42_38_00
    { 0x3C2A2700, "Stormhill - North of Warmaster's Shack" },   // m60_42_39_00
    { 0x3C2A2800, "Stormhill - Northwest Cliffside" },   // m60_42_40_00
    { 0x3C2A3200, "Leyndell - South of Outer Wall Phantom Tree" },   // m60_42_50_00
    { 0x3C2A3300, "Leyndell - Outer Wall Phantom Tree" },   // m60_42_51_00
    { 0x3C2A3400, "Leyndell - Southwest Outer Wall Battleground" },   // m60_42_52_00
    { 0x3C2A3500, "Leyndell - Northwest Outer Wall Battleground" },   // m60_42_53_00
    { 0x3C2A3600, "Altus Plateau - East Windmill Village, Highway Lookout Tower" },   // m60_42_54_00
    { 0x3C2A3700, "Altus Plateau - Windmill Heights" },   // m60_42_55_00
    { 0x3C2B1E00, "Weeping Peninsula - Morne Moangrave" },   // m60_43_30_00
    { 0x3C2B1F00, "Weeping Peninsula - Castle Morne" },   // m60_43_31_00
    { 0x3C2B2000, "Weeping Peninsula - Castle Morne Approach Northwest Cliffside" },   // m60_43_32_00
    { 0x3C2B2100, "Weeping Peninsula - Minor Erdtree, Tombsward Catacombs Entrance, Morne Tunnel Entrance" },   // m60_43_33_00
    { 0x3C2B2200, "Weeping Peninsula - Church of Pilgrimage, Demi-Human Forest Ruins" },   // m60_43_34_00
    { 0x3C2B2300, "Limgrave - Seaside Ruins" },   // m60_43_35_00
    { 0x3C2B2400, "Limgrave - Dragon-Burnt Ruins" },   // m60_43_36_00
    { 0x3C2B2500, "Limgrave - Agheel Lake North, Murkwater Cave Entrance" },   // m60_43_37_00
    { 0x3C2B2600, "Limgrave - Murkwater Coast, Murkwater Catacombs Entrance" },   // m60_43_38_00
    { 0x3C2B2700, "Stormhill - Saintsbridge, Deathtouched Catacombs Entrance, Highroad Cave Entrance" },   // m60_43_39_00
    { 0x3C2B2800, "Stormhill - Northeast Cliffside" },   // m60_43_40_00
    { 0x3C2B3200, "Leyndell - Minor Erdtree Church, Sealed Tunnel Entrance" },   // m60_43_50_00
    { 0x3C2B3300, "Leyndell - East of Outer Wall Phantom Tree" },   // m60_43_51_00
    { 0x3C2B3400, "Leyndell - Southeast Outer Wall Battleground" },   // m60_43_52_00
    { 0x3C2B3500, "Leyndell - Northeast Outer Wall Battleground, Hermit Merchant's Shack" },   // m60_43_53_00
    { 0x3C2B3600, "Altus Plateau - Cliffside Northeast of Outer Wall Battlefield" },   // m60_43_54_00
    { 0x3C2C1F00, "Weeping Peninsula - Castle Morne Approach South" },   // m60_44_31_00
    { 0x3C2C2000, "Weeping Peninsula - Castle Morne Approach North" },   // m60_44_32_00
    { 0x3C2C2100, "Weeping Peninsula - Castle Morne Rampart, Ailing Village Outskirts" },   // m60_44_33_00
    { 0x3C2C2200, "Weeping Peninsula - Bridge of Sacrifice, Forest Lookout Tower, Earthbore Cave Entrance" },   // m60_44_34_00
    { 0x3C2C2300, "Limgrave - Agheel Lake South, Forlorn Hound Evergaol" },   // m60_44_35_00
    { 0x3C2C2400, "Limgrave - Waypoint Ruins" },   // m60_44_36_00
    { 0x3C2C2500, "Limgrave - Mistwood Outskirts" },   // m60_44_37_00
    { 0x3C2C2600, "Limgrave - Artist's Shack" },   // m60_44_38_00
    { 0x3C2C2700, "Limgrave - Summonwater Village Outskirts" },   // m60_44_39_00
    { 0x3C2C3400, "Leyndell - South of Minor Erdtree" },   // m60_44_52_00
    { 0x3C2C3500, "Leyndell - Minor Erdtree" },   // m60_44_53_00
    { 0x3C2D2000, "Weeping Peninsula - Castle Morne Approach Northeast Cliffside" },   // m60_45_32_00
    { 0x3C2D2100, "Weeping Peninsula - Beside the Crater-Pocked Glade, Oridys's Rise" },   // m60_45_33_00
    { 0x3C2D2200, "Weeping Peninsula - Impaler's Catacombs Entrance" },   // m60_45_34_00
    { 0x3C2D2300, "Limgrave - Southwest of Fort Haight" },   // m60_45_35_00
    { 0x3C2D2400, "Limgrave - Fort Haight West" },   // m60_45_36_00
    { 0x3C2D2500, "Limgrave - Siofra River Well, Mistwood Ruins, Minor Erdtree, Nokron Entrance" },   // m60_45_37_00
    { 0x3C2D2600, "Limgrave - South of Summonwater Village" },   // m60_45_38_00
    { 0x3C2D2700, "Limgrave - Summonwater Village" },   // m60_45_39_00
    { 0x3C2D2800, "Limgrave - North of Summonwater Village" },   // m60_45_40_00
    { 0x3C2D3300, "Leyndell - Auriza Hero's Grave Entrance" },   // m60_45_51_00
    { 0x3C2D3400, "Leyndell - Capital Rampart, Auriza Side Tomb Entrance" },   // m60_45_52_00
    { 0x3C2D3500, "Leyndell - Capital Rampart North Cliffside" },   // m60_45_53_00
    { 0x3C2E2400, "Limgrave - Fort Haight" },   // m60_46_36_00
    { 0x3C2E2500, "Limgrave - East of Siofra River Well" },   // m60_46_37_00
    { 0x3C2E2600, "Limgrave - Third Church of Marika, Gaol Cave Exit" },   // m60_46_38_00
    { 0x3C2E2700, "Limgrave - Rear Gael Tunnel Entrance; Caelid - Gael Tunnel Entrance" },   // m60_46_39_00
    { 0x3C2E2800, "Caelid - Smoldering Church, Rotview Balcony" },   // m60_46_40_00
    { 0x3C2E3700, "Consecrated Snowfield" },   // m60_46_55_00
    { 0x3C2E3900, "Consecrated Snowfield - Far West Cliffside" },   // m60_46_57_00
    { 0x3C2F2500, "Caelid - West of Caelid Highway South" },   // m60_47_37_00
    { 0x3C2F2600, "Caelid - Fort Gael, Caelid Waypoint Ruins, Gaol Cave Entrance" },   // m60_47_38_00
    { 0x3C2F2700, "Caelid - Fort Gael North" },   // m60_47_39_00
    { 0x3C2F2800, "Caelid - Caelem Ruins, Forsaken Ruins, Minor Erdtree, Minor Erdtree Catacombs Entrance" },   // m60_47_40_00
    { 0x3C2F2900, "Caelid - South Great-Jar Area" },   // m60_47_41_00
    { 0x3C2F2A00, "Caelid - North Great-Jar Area" },   // m60_47_42_00
    { 0x3C2F3300, "Leyndell - Divine Tower of East Altus Entrance; Mountaintops of the Giants - Forbidden Lands Start" },   // m60_47_51_00
    { 0x3C2F3700, "Consecrated Snowfield - Yelough Anix Ruins, Yelough Anix Tunnel Entrance" },   // m60_47_55_00
    { 0x3C2F3800, "Consecrated Snowfield - North of Yelough Anix Ruins" },   // m60_47_56_00
    { 0x3C2F3900, "Consecrated Snowfield - West of Ordina" },   // m60_47_57_00
    { 0x3C2F3A00, "Consecrated Snowfield - Apostate Derelict" },   // m60_47_58_00
    { 0x3C302400, "Caelid - Cathedral of Dragon Communion, Caelid Catacombs Entrance" },   // m60_48_36_00
    { 0x3C302500, "Caelid - Caelid Highway South" },   // m60_48_37_00
    { 0x3C302600, "Caelid - West Aeonia Swamp" },   // m60_48_38_00
    { 0x3C302700, "Caelid - Smoldering Wall, Abandoned Cave Entrance" },   // m60_48_39_00
    { 0x3C302800, "Caelid - Deep Siofra Well; Greyoll's Dragonbarrow - Dragonbarrow West" },   // m60_48_40_00
    { 0x3C302900, "Greyoll's Dragonbarrow - Isolated Merchant's Shack" },   // m60_48_41_00
    { 0x3C303300, "Mountaintops of the Giants - Forbidden Lands Midway" },   // m60_48_51_00
    { 0x3C303600, "Consecrated Snowfield - Southwest Foggy Area" },   // m60_48_54_00
    { 0x3C303700, "Consecrated Snowfield - Northwest Foggy Area, East of Yelough Anix Ruins" },   // m60_48_55_00
    { 0x3C303800, "Consecrated Snowfield - South of Ordina" },   // m60_48_56_00
    { 0x3C303900, "Consecrated Snowfield - Ordina, Liturgical Town" },   // m60_48_57_00
    { 0x3C303A00, "Consecrated Snowfield - East of Apostate Derelict" },   // m60_48_58_00
    { 0x3C312400, "Caelid - Southwest of Caelid Highway South" },   // m60_49_36_00
    { 0x3C312500, "Caelid - Southern Aeonia Swamp Bank" },   // m60_49_37_00
    { 0x3C312600, "Caelid - East Aeonia Swamp" },   // m60_49_38_00
    { 0x3C312700, "Caelid - West Sellia, Sellia Crystal Tunnel Entrance" },   // m60_49_39_00
    { 0x3C312800, "Greyoll's Dragonbarrow - South of Divine Tower of Caelid, Deep Siofra Well" },   // m60_49_40_00
    { 0x3C312900, "Greyoll's Dragonbarrow - Divine Tower of Caelid Entrance" },   // m60_49_41_00
    { 0x3C313400, "Mountaintops of the Giants - Before Grand Lift of Rold" },   // m60_49_52_00
    { 0x3C313500, "Mountaintops of the Giants - West Zamor Ruins, Grand Lift of Rold" },   // m60_49_53_00
    { 0x3C313600, "Consecrated Snowfield - Hidden Path to the Haligtree, Southeast Foggy Area" },   // m60_49_54_00
    { 0x3C313700, "Consecrated Snowfield - Northeast Foggy Area, Inner Consecrated Snowfield" },   // m60_49_55_00
    { 0x3C313800, "Consecrated Snowfield - Southeast of Ordina" },   // m60_49_56_00
    { 0x3C313900, "Consecrated Snowfield - East of Ordina" },   // m60_49_57_00
    { 0x3C322400, "Caelid - Impassable Greatbridge" },   // m60_50_36_00
    { 0x3C322500, "Caelid - Far Southwest Radahn Arena" },   // m60_50_37_00
    { 0x3C322600, "Caelid - Gowry's Shack" },   // m60_50_38_00
    { 0x3C322700, "Caelid - East Sellia, Church of the Plague" },   // m60_50_39_00
    { 0x3C322800, "Greyoll's Dragonbarrow - Dragonbarrow Fork" },   // m60_50_40_00
    { 0x3C322900, "Greyoll's Dragonbarrow - North of Dragonbarrow Fork" },   // m60_50_41_00
    { 0x3C323500, "Mountaintops of the Giants - East Zamor Ruins" },   // m60_50_53_00
    { 0x3C323600, "Mountaintops of the Giants - North of Zamor Ruins" },   // m60_50_54_00
    { 0x3C323700, "Consecrated Snowfield - Consecrated Snowfield Catacombs Entrance" },   // m60_50_55_00
    { 0x3C323800, "Mountaintops of the Giants - Shack of the Lofty; Consecrated Snowfield - Minor Erdtree, Albinauric Rise, Cave of the Forlorn Entrance" },   // m60_50_56_00
    { 0x3C323900, "Mountaintops of the Giants - West of Castle Sol; Consecrated Snowfield - North of Minor Erdtree" },   // m60_50_57_00
    { 0x3C332300, "Caelid - Redmane Castle South Cliffside" },   // m60_51_35_00
    { 0x3C332400, "Caelid - Redmane Castle" },   // m60_51_36_00
    { 0x3C332500, "Caelid - Southwest Radahn Arena" },   // m60_51_37_00
    { 0x3C332600, "Caelid - West Radahn Arena" },   // m60_51_38_00
    { 0x3C332700, "Greyoll's Dragonbarrow - Fort Faroth" },   // m60_51_39_00
    { 0x3C332800, "Greyoll's Dragonbarrow - Minor Erdtree, Dragonbarrow Cave Entrance" },   // m60_51_40_00
    { 0x3C332900, "Greyoll's Dragonbarrow - Southwest Farum Greatbridge" },   // m60_51_41_00
    { 0x3C332A00, "Greyoll's Dragonbarrow - Northwest Farum Greatbridge" },   // m60_51_42_00
    { 0x3C332B00, "Greyoll's Dragonbarrow - Bestial Sanctum" },   // m60_51_43_00
    { 0x3C333400, "Mountaintops of the Giants - Giants' Gravepost South Cliffside" },   // m60_51_52_00
    { 0x3C333500, "Mountaintops of the Giants - Southwest Giants' Gravepost" },   // m60_51_53_00
    { 0x3C333600, "Mountaintops of the Giants - Northwest Giants' Gravepost" },   // m60_51_54_00
    { 0x3C333700, "Mountaintops of the Giants - Before Ancient Snow Valley Ruins" },   // m60_51_55_00
    { 0x3C333800, "Mountaintops of the Giants - Ancient Snow Valley Ruins, Stargazers' Ruins" },   // m60_51_56_00
    { 0x3C333900, "Mountaintops of the Giants - South Castle Sol, Snow Valley Ruins Overlook" },   // m60_51_57_00
    { 0x3C333A00, "Mountaintops of the Giants - North Castle Sol" },   // m60_51_58_00
    { 0x3C342500, "Caelid - South Radahn Arena" },   // m60_52_37_00
    { 0x3C342600, "Caelid - Central Radahn Arena" },   // m60_52_38_00
    { 0x3C342700, "Caelid - North Radahn Arena" },   // m60_52_39_00
    { 0x3C342800, "Caelid - Far North Radahn Arena" },   // m60_52_40_00
    { 0x3C342900, "Greyoll's Dragonbarrow - Southeast Farum Greatbridge, Lenne's Rise" },   // m60_52_41_00
    { 0x3C342A00, "Greyoll's Dragonbarrow - Northeast Farum Greatbridge" },   // m60_52_42_00
    { 0x3C342B00, "Greyoll's Dragonbarrow - Northeast Cliffside" },   // m60_52_43_00
    { 0x3C343400, "Mountaintops of the Giants - Southwest Fire Giant Arena" },   // m60_52_52_00
    { 0x3C343500, "Mountaintops of the Giants - Northwest Fire Giant Arena, Foot of the Forge" },   // m60_52_53_00
    { 0x3C343600, "Mountaintops of the Giants - Northeast Giants' Gravepost" },   // m60_52_54_00
    { 0x3C343700, "Mountaintops of the Giants - Guardians' Garrison" },   // m60_52_55_00
    { 0x3C343800, "Mountaintops of the Giants - Whiteridge Road, Minor Erdtree" },   // m60_52_56_00
    { 0x3C343900, "Mountaintops of the Giants - Before Freezing Lake, Heretical Rise" },   // m60_52_57_00
    { 0x3C343A00, "Mountaintops of the Giants - Northwest of Freezing Lake, East of Castle Sol" },   // m60_52_58_00
    { 0x3C352600, "Caelid - East Radahn Arena" },   // m60_53_38_00
    { 0x3C352700, "Caelid - Northeast Radahn Arena" },   // m60_53_39_00
    { 0x3C353400, "Mountaintops of the Giants - Southeast Fire Giant Arena" },   // m60_53_52_00
    { 0x3C353500, "Mountaintops of the Giants - Northeast Fire Giant Arena" },   // m60_53_53_00
    { 0x3C353600, "Mountaintops of the Giants - Giants' Gravepost Northeast Cliffside" },   // m60_53_54_00
    { 0x3C353700, "Mountaintops of the Giants - West of First Church of Marika" },   // m60_53_55_00
    { 0x3C353800, "Mountaintops of the Giants - Southwest Freezing Lake, Lord Contender's Evergaol, Spiritcaller's Cave Entrance" },   // m60_53_56_00
    { 0x3C353900, "Mountaintops of the Giants - Northwest Freezing Lake" },   // m60_53_57_00
    { 0x3C353A00, "Mountaintops of the Giants - Freezing Lake North Cliffside" },   // m60_53_58_00
    { 0x3C363500, "Mountaintops of the Giants - Forge of the Giants" },   // m60_54_53_00
    { 0x3C363700, "Mountaintops of the Giants - First Church of Marika" },   // m60_54_55_00
    { 0x3C363800, "Mountaintops of the Giants - Southeast Freezing Lake" },   // m60_54_56_00
    { 0x3C363900, "Mountaintops of the Giants - Northeast Freezing Lake" },   // m60_54_57_00
    { 0x3D2C2900, "Gravesite Plain" },   // m61_44_41_00
    { 0x3D2C2B00, "Gravesite Plain" },   // m61_44_43_00
    { 0x3D2C2D00, "Ancient Ruins of Rauh" },   // m61_44_45_00
    { 0x3D2C2E00, "Rauh Base" },   // m61_44_46_00
    { 0x3D2C2F00, "Rauh Base" },   // m61_44_47_00
    { 0x3D2C3000, "Rauh Base" },   // m61_44_48_00
    { 0x3D2D2700, "Charo's Hidden Grave" },   // m61_45_39_00
    { 0x3D2D2800, "Gravesite Plain" },   // m61_45_40_00
    { 0x3D2D2900, "Gravesite Plain" },   // m61_45_41_00
    { 0x3D2D2A00, "Gravesite Plain" },   // m61_45_42_00
    { 0x3D2D2B00, "Gravesite Plain" },   // m61_45_43_00
    { 0x3D2D2C00, "Gravesite Plain" },   // m61_45_44_00
    { 0x3D2D2D00, "Rauh Base" },   // m61_45_45_00
    { 0x3D2D2E00, "Rauh Base" },   // m61_45_46_00
    { 0x3D2D2F00, "Rauh Base" },   // m61_45_47_00
    { 0x3D2D3000, "Rauh Base" },   // m61_45_48_00
    { 0x3D2E2300, "Cerulean Coast" },   // m61_46_35_00
    { 0x3D2E2600, "Cerulean Coast" },   // m61_46_38_00
    { 0x3D2E2700, "Cerulean Coast" },   // m61_46_39_00
    { 0x3D2E2800, "Gravesite Plain" },   // m61_46_40_00
    { 0x3D2E2900, "Gravesite Plain" },   // m61_46_41_00
    { 0x3D2E2A00, "Gravesite Plain" },   // m61_46_42_00
    { 0x3D2E2B00, "Gravesite Plain" },   // m61_46_43_00
    { 0x3D2E2C00, "Gravesite Plain" },   // m61_46_44_00
    { 0x3D2E2D00, "Rauh Base" },   // m61_46_45_00
    { 0x3D2E2E00, "Rauh Base" },   // m61_46_46_00
    { 0x3D2E2F00, "Rauh Base" },   // m61_46_47_00
    { 0x3D2E3000, "Ancient Ruins of Rauh" },   // m61_46_48_00
    { 0x3D2F2300, "Cerulean Coast" },   // m61_47_35_00
    { 0x3D2F2400, "Cerulean Coast" },   // m61_47_36_00
    { 0x3D2F2500, "Cerulean Coast" },   // m61_47_37_00
    { 0x3D2F2600, "Cerulean Coast" },   // m61_47_38_00
    { 0x3D2F2700, "Cerulean Coast" },   // m61_47_39_00
    { 0x3D2F2800, "Gravesite Plain" },   // m61_47_40_00
    { 0x3D2F2900, "Gravesite Plain" },   // m61_47_41_00
    { 0x3D2F2A00, "Gravesite Plain" },   // m61_47_42_00
    { 0x3D2F2B00, "Gravesite Plain" },   // m61_47_43_00
    { 0x3D2F2C00, "Gravesite Plain" },   // m61_47_44_00
    { 0x3D2F2D00, "Gravesite Plain" },   // m61_47_45_00
    { 0x3D2F2E00, "Scadu Altus" },   // m61_47_46_00
    { 0x3D2F2F00, "Rauh Base" },   // m61_47_47_00
    { 0x3D2F3000, "Rauh Base" },   // m61_47_48_00
    { 0x3D302500, "Cerulean Coast" },   // m61_48_37_00
    { 0x3D302600, "Cerulean Coast" },   // m61_48_38_00
    { 0x3D302700, "Cerulean Coast" },   // m61_48_39_00
    { 0x3D302800, "Gravesite Plain" },   // m61_48_40_00
    { 0x3D302900, "Gravesite Plain" },   // m61_48_41_00
    { 0x3D302A00, "Gravesite Plain" },   // m61_48_42_00
    { 0x3D302B00, "Gravesite Plain" },   // m61_48_43_00
    { 0x3D302C00, "Gravesite Plain" },   // m61_48_44_00
    { 0x3D302D00, "Scadu Altus" },   // m61_48_45_00
    { 0x3D302E00, "Scadu Altus" },   // m61_48_46_00
    { 0x3D302F00, "Scadu Altus" },   // m61_48_47_00
    { 0x3D303000, "Rauh Base" },   // m61_48_48_00
    { 0x3D303100, "Scaduview" },   // m61_48_49_00
    { 0x3D312500, "Cerulean Coast" },   // m61_49_37_00
    { 0x3D312600, "Cerulean Coast" },   // m61_49_38_00
    { 0x3D312700, "Finger Ruins of Rhia" },   // m61_49_39_00
    { 0x3D312800, "Foot of the Jagged Peak" },   // m61_49_40_00
    { 0x3D312900, "Foot of the Jagged Peak" },   // m61_49_41_00
    { 0x3D312A00, "Gravesite Plain" },   // m61_49_42_00
    { 0x3D312B00, "Gravesite Plain" },   // m61_49_43_00
    { 0x3D312C00, "Scadu Altus" },   // m61_49_44_00
    { 0x3D312D00, "Scadu Altus" },   // m61_49_45_00
    { 0x3D312E00, "Scadu Altus" },   // m61_49_46_00
    { 0x3D312F00, "Scadu Altus" },   // m61_49_47_00
    { 0x3D313000, "Scaduview" },   // m61_49_48_00
    { 0x3D313100, "Scaduview" },   // m61_49_49_00
    { 0x3D322500, "Cerulean Coast" },   // m61_50_37_00
    { 0x3D322600, "Finger Ruins of Rhia" },   // m61_50_38_00
    { 0x3D322700, "Finger Ruins of Rhia" },   // m61_50_39_00
    { 0x3D322800, "Finger Ruins of Rhia" },   // m61_50_40_00
    { 0x3D322900, "Finger Ruins of Rhia" },   // m61_50_41_00
    { 0x3D322A00, "Abyssal Woods" },   // m61_50_42_00
    { 0x3D322B00, "Scadu Altus" },   // m61_50_43_00
    { 0x3D322C00, "Scadu Altus" },   // m61_50_44_00
    { 0x3D322D00, "Scadu Altus" },   // m61_50_45_00
    { 0x3D322E00, "Scadu Altus" },   // m61_50_46_00
    { 0x3D322F00, "Scadu Altus" },   // m61_50_47_00
    { 0x3D323000, "Scaduview" },   // m61_50_48_00
    { 0x3D323100, "Scaduview" },   // m61_50_49_00
    { 0x3D332600, "Finger Ruins of Rhia" },   // m61_51_38_00
    { 0x3D332700, "Finger Ruins of Rhia" },   // m61_51_39_00
    { 0x3D332800, "Finger Ruins of Rhia" },   // m61_51_40_00
    { 0x3D332900, "Finger Ruins of Rhia" },   // m61_51_41_00
    { 0x3D332A00, "Abyssal Woods" },   // m61_51_42_00
    { 0x3D332B00, "Scadu Altus" },   // m61_51_43_00
    { 0x3D332C00, "Scadu Altus" },   // m61_51_44_00
    { 0x3D332D00, "Scadu Altus" },   // m61_51_45_00
    { 0x3D332E00, "Scadu Altus" },   // m61_51_46_00
    { 0x3D332F00, "Scadu Altus" },   // m61_51_47_00
    { 0x3D333000, "Scaduview" },   // m61_51_48_00
    { 0x3D333100, "Scaduview" },   // m61_51_49_00
    { 0x3D342700, "Foot of the Jagged Peak" },   // m61_52_39_00
    { 0x3D342800, "Foot of the Jagged Peak" },   // m61_52_40_00
    { 0x3D342900, "Abyssal Woods" },   // m61_52_41_00
    { 0x3D342A00, "Abyssal Woods" },   // m61_52_42_00
    { 0x3D342B00, "Abyssal Woods" },   // m61_52_43_00
    { 0x3D342D00, "Scadu Altus" },   // m61_52_45_00
    { 0x3D342E00, "Scadu Altus" },   // m61_52_46_00
    { 0x3D342F00, "Scadu Altus" },   // m61_52_47_00
    { 0x3D343000, "Scaduview" },   // m61_52_48_00
    { 0x3D343100, "Scaduview" },   // m61_52_49_00
    { 0x3D352700, "Jagged Peak" },   // m61_53_39_00
    { 0x3D352800, "Abyssal Woods" },   // m61_53_40_00
    { 0x3D352900, "Abyssal Woods" },   // m61_53_41_00
    { 0x3D352D00, "Finger Ruins of Dheo" },   // m61_53_45_00
    { 0x3D352E00, "Finger Ruins of Dheo" },   // m61_53_46_00
    { 0x3D352F00, "Finger Ruins of Dheo" },   // m61_53_47_00
    { 0x3D353000, "Scaduview" },   // m61_53_48_00
    { 0x3D362700, "Jagged Peak" },   // m61_54_39_00
    { 0x3D362800, "Jagged Peak" },   // m61_54_40_00
    { 0x3D362D00, "Finger Ruins of Dheo" },   // m61_54_45_00
    { 0x3D362E00, "Finger Ruins of Dheo" },   // m61_54_46_00
    { 0x3D362F00, "Finger Ruins of Dheo" },   // m61_54_47_00
    { 0x3D363000, "Scaduview" },   // m61_54_48_00
    { 0x3D372700, "Jagged Peak" },   // m61_55_39_00
};

struct KindEntry { int area; const char* name; };
const KindEntry kKinds[] = {
    { 12, "Underground" }, { 30, "Catacombs" }, { 31, "Cave" }, { 32, "Tunnel" }, { 34, "Divine Tower" },
    { 40, "Catacombs (Realm of Shadow)" }, { 41, "Gaol" }, { 42, "Ruined Forge" }, { 43, "Cave (Realm of Shadow)" },
    { 45, "Colosseum" }, { 60, "The Lands Between" }, { 61, "Realm of Shadow" },
};

const char* Lookup(uint32_t id)
{
    auto it = std::lower_bound(std::begin(kMaps), std::end(kMaps), id,
                               [](const MapEntry& e, uint32_t v) { return e.id < v; });
    return (it != std::end(kMaps) && it->id == id) ? it->name : nullptr;
}

} // namespace

std::string ErMapIdText(uint32_t id)
{
    char b[24];
    snprintf(b, sizeof(b), "m%02u_%02u_%02u_%02u", id >> 24, (id >> 16) & 0xFF, (id >> 8) & 0xFF, id & 0xFF);
    return b;
}

bool ErMapIdValid(uint32_t id)
{
    unsigned a = id >> 24, b = (id >> 16) & 0xFF, c = (id >> 8) & 0xFF, d = id & 0xFF;
    if (id == 0 || id == 0xFFFFFFFF) return false;
    if (a == 60 || a == 61) return b < 100 && c < 100 && d < 100;
    return a >= 10 && a <= 99 && b < 100 && c < 100 && d < 100;
}

std::string ErMapName(uint32_t id)
{
    if (!ErMapIdValid(id)) return {};
    if (const char* n = Lookup(id)) return n;
    unsigned a = id >> 24;
    // Alternate versions of a map (e.g. ashen or meteored tiles, _10/_12) share the base tile's name.
    if (const char* n = Lookup(id & 0xFFFFFF00u)) return n;
    if (a != 60 && a != 61)
        if (const char* n = Lookup(id & 0xFFFF0000u)) return n;
    char b[96];
    for (const auto& k : kKinds) {
        if ((int)a == k.area) {
            snprintf(b, sizeof(b), "%s (%s)", k.name, ErMapIdText(id).substr(0, 6).c_str());
            return b;
        }
    }
    return "Mapa " + ErMapIdText(id);
}

std::string ErRegionOf(const std::string& area)
{
    size_t p = area.find(" - ");
    return p == std::string::npos ? area : area.substr(0, p);
}

uint32_t ErEntityMap(uint32_t e)
{
    if (e >= 1000000000u) {                 // 10XXYYnnnn (m60) / 20XXYYnnnn (m61)
        unsigned prefix = e / 100000000u, x = (e / 1000000u) % 100, y = (e / 10000u) % 100;
        unsigned area = prefix == 10 ? 60 : prefix == 20 ? 61 : 0;
        return area ? (area << 24 | x << 16 | y << 8) : 0;
    }
    if (e >= 10000000u && e < 100000000u)   // AABBnnnn (legacy dungeons and dungeons)
        return (e / 1000000u) << 24 | ((e / 10000u) % 100) << 16;
    return 0;
}

std::string ErGraceLabel(uint32_t e)
{
    if (!e) return {};
    uint32_t map = ErEntityMap(e);
    std::string name = map ? ErMapName(map) : std::string();
    return name.empty() ? "#" + std::to_string(e) : name;
}

const std::vector<std::string>& ErAllMapNames()
{
    static const std::vector<std::string> list = [] {
        std::set<std::string> names;
        for (const auto& m : kMaps) names.insert(m.name);
        return std::vector<std::string>(names.begin(), names.end());
    }();
    return list;
}

const std::vector<std::string>& ErCauseSuggestions()
{
    static const std::vector<std::string> list = {
        "Queda", "Armadilha", "Invasor (PvP)", "Inimigo comum", "Grupo de inimigos",
        "Podridão escarlate", "Hemorragia", "Congelamento (Frostbite)", "Veneno", "Morte instantânea (Deathblight)",
        "Loucura (Madness)", "Sono",
        "Chefe: Margit, the Fell Omen", "Chefe: Godrick the Grafted", "Chefe: Tree Sentinel",
        "Chefe: Leonine Misbegotten", "Chefe: Royal Knight Loretta", "Chefe: Red Wolf of Radagon",
        "Chefe: Rennala, Queen of the Full Moon", "Chefe: Starscourge Radahn", "Chefe: Godskin Noble",
        "Chefe: Rykard, Lord of Blasphemy", "Chefe: Morgott, the Omen King", "Chefe: Fire Giant",
        "Chefe: Godskin Duo", "Chefe: Maliketh, the Black Blade", "Chefe: Hoarah Loux, Warrior",
        "Chefe: Radagon of the Golden Order", "Chefe: Elden Beast", "Chefe: Malenia, Blade of Miquella",
        "Chefe: Mohg, Lord of Blood", "Chefe: Dragonlord Placidusax", "Chefe: Astel, Naturalborn of the Void",
        "Chefe: Mimic Tear", "Chefe: Regal Ancestor Spirit", "Chefe: Lichdragon Fortissax",
        "Chefe: Commander Niall", "Chefe: Loretta, Knight of the Haligtree", "Chefe: Draconic Tree Sentinel",
        "Chefe: Night's Cavalry", "Chefe: Erdtree Avatar", "Chefe: Crucible Knight", "Chefe: Black Knife Assassin",
        "Chefe: Ulcerated Tree Spirit", "Chefe: Dragonkin Soldier", "Chefe: Fallingstar Beast",
        "Chefe: Divine Beast Dancing Lion", "Chefe: Rellana, Twin Moon Knight", "Chefe: Messmer the Impaler",
        "Chefe: Commander Gaius", "Chefe: Scadutree Avatar", "Chefe: Putrescent Knight",
        "Chefe: Romina, Saint of the Bud", "Chefe: Midra, Lord of Frenzied Flame", "Chefe: Bayle the Dread",
        "Chefe: Metyr, Mother of Fingers", "Chefe: Promised Consort Radahn",
        "Dragão", "Troll", "Cão / Rato", "Soldado",
    };
    return list;
}
