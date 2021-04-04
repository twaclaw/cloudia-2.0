git submodule add https://github.com/mkuyper/basicmac
git submodule update --init --recursive

python -m venv venv
source venv/bin/activate

pip install -r basicmac/requirements.txt
pip install -r basicmac/basicloader/requirements.txt
pip install pyyaml
