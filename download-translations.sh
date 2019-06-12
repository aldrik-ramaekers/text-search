#!/bin/bash

# download translations
# https://poeditor.com/projects/

code=$(curl -X POST https://poeditor.com/api/ \
     -d api_token="89856d745c143ad91aca0f91c9e7640b" \
     -d action="export" \
     -d id="267905" \
     -d language="en" \
     -d type="mo" | jq -r '.item')

wget -O data/translations/en.mo "$code"