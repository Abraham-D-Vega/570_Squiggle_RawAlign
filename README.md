Welcome to SquiggleSeeder!

To compute hash tables:

    1. Navigate to root directory
    
    2. Run "sh SquiggleSeeder/scripts/GenHashTable.sh <genome>"
    
    <genome> can be any of the ones available in "data/" (i.e. covid)
    
    3. Output hash tables will appear in "SquiggleSeeder/hashtables/"

    You may change parameters in "SquiggleSeeder/params.hpp" (default ones are chosen due to compatibility or from RawHash)
    
    Tiling is available to generate hash tables over sections of the reference genome (set IS_TILED to true)
