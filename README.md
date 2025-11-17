Welcome to SquiggleSeeder!

Prerequisites:
    
    Create virtual environment to install dependencies (run following commands):

    Navigate to root directory

    1. python3 -m venv venv

    2. source venv/bin/activate

    3. pip install -r requirements.txt

To run preprocessing steps (compute hash tables and normalized 8-bit reference event signals):

    Navigate to root directory
    
    1. Run "sh SquiggleSeeder/scripts/preprocess.sh <genome>"
    
    <genome> can be any of the ones available in "data/" (i.e. covid)
    
    Output hash tables will appear in "SquiggleSeeder/hash_tables/"

    Note: You may change parameters in "SquiggleSeeder/params.hpp" (default ones are chosen due to compatibility or from RawHash).
    Tiling is used to generate hash tables over sections of the reference genome (set IS_TILED to true)
