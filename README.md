Welcome to SquiggleSeeder!

Prerequisites:

Create virtual environment to install dependencies (run following commands):

    Navigate to root directory

    1. python3 -m venv venv

    2. source venv/bin/activate

    3. pip install -r requirements.txt

To simulate SquiggleFilter:

    Navigate to root directory
    
    1. Run "sh SquiggleSeeder/scripts/simulate_squiggle_filter.sh <genome>"

    <genome> can be any of the ones available in "data/" (i.e. covid). Make sure you downloaded fast5 files first for that genome. They would be stored in "data/<genome>/fast5/".

    Results will show up in "results/squiggle_filter/".

To simulate SquiggleSeeder:

    Navigate to root directory
    
    1. Run "sh SquiggleSeeder/scripts/simulate_seeder.sh <genome> --align"

    <genome> can be any of the ones available in "data/" (i.e. covid). Make sure you downloaded fast5 files first for that genome. They would be stored in "data/<genome>/fast5/".

    Results will show up in "results/seeder/".

    Optionally take out the "--align" flag if you don't want to run alignment. 
    Results will be in "<genome>_align.txt"

Note: You may change parameters in "SquiggleSeeder/utils.hpp" (default ones are chosen due to compatibility or from RawHash). Codebase is currently not compatible with setting IS_TILING to true.