#! /usr/bin/python3

import subprocess
import sys
import os
import json
import requests


def download_file(url, output=None):
    if not output:
        output = url.split('/')
        output = '/app/tools/cached_' + output[len(output) - 1]
    if os.path.exists(output):
        print('Already existing file', output)
        return output
    r = requests.get(url)
    assert r.status_code == 200, 'Error getting file'
    with open(output, 'wb') as handle:
        handle.write(r.content)
    return output

url_api = 'https://catalog.metronaut.app/v2/all-no-cache'
url_api_piece = 'https://catalog.metronaut.app/v2/pieces/'

data_folder = '/app/tools/'
v2_all_store = os.path.join(data_folder, 'v2_all_store.json')
processing_store_file = os.path.join(data_folder, 'processing_store.json')

# We fetch v2 all if it doesnt already exists locally
if not os.path.exists(v2_all_store):
    print('Fetching v2 all')
    r = requests.get(url_api, headers={'Authorization': b'Basic YXBwOm1ldHJvbmF1dDE5MjU='})
    if r.status_code != 200:
        print('Error while fetching v2 all', r.status_code)
        sys.exit(1)
    with open(v2_all_store, 'wb') as handle:
        handle.write(r.text.encode('utf-8'))

# Load local v2 all
v2_all = None
with open(v2_all_store, 'rb') as handle:
    v2_all = handle.read().decode('ascii', 'ignore')
    v2_all = json.loads(v2_all)

processing_store = {'processed_pieces': [], 'processed_accompaniments': []}

if os.path.exists(processing_store_file):
    with open(processing_store_file, 'r') as handle:
        processing_store = json.load(handle)


accomp_pk_to_piece_pk = {}
piece_pk_to_accomp_pk = {}

for piece in v2_all['piece']:
    piece_pk = piece['pk']
    piece_pk_to_accomp_pk[piece_pk] = []
    for accomp in piece['accompaniments']:
        accomp_pk_to_piece_pk[accomp['pk']] = piece_pk
        piece_pk_to_accomp_pk[piece_pk].append(accomp['pk'])

accomp_pk = None
piece_pk = None

# An accompaniment pk is provided
if len(sys.argv) >= 2:
    accomp_pk = int(sys.argv[1])
    if accomp_pk in accomp_pk_to_piece_pk:
        piece_pk = accomp_pk_to_piece_pk[accomp_pk]
    else:
        print('No piece could be found for this accompaniment pk')
        sys.exit(1)
else:
    for piece in v2_all['piece']:
        if piece['status'] != 'Ready':
            continue
        if piece['pk'] in processing_store['processed_pieces']:
            continue
        if not piece['accompaniments']:
            continue
        piece_pk = piece['pk']
        break

# We can remove this security later
if piece_pk in processing_store['processed_pieces']:
    print('Piece', piece_pk, 'is already processed')
    sys.exit(1)

if accomp_pk in processing_store['processed_accompaniments']:
    print('Accompaniment', accomp_pk, 'is already processed')
    sys.exit(1)

if not piece_pk:
    print('No piece_pk could be found. Abort')
    sys.exit(1)

# Call on v2/piece/{pk}
cache_path = 'cached_piece_' + str(piece_pk) + '.json'
piece_detail = None

if os.path.exists(cache_path):
    with open(cache_path, 'rb') as handle:
        piece_detail = handle.read().decode('ascii', 'ignore')
        piece_detail = json.loads(piece_detail)
else:
    r = requests.get(url_api_piece + str(piece_pk), headers={'Authorization': b'Basic YXBwOm1ldHJvbmF1dDE5MjU='})
    if r.status_code != 200:
        print('Error while fetching piece', r.status_code)
        sys.exit(1)
    with open(cache_path, 'wb') as handle:
        handle.write(r.text.encode('utf-8'))
    piece_detail = r.json()

if accomp_pk is None:
    for accomp in piece_detail['accompaniments']:
        accomp_pk = accomp['pk']
        break

if not accomp_pk:
    print('No accomp_pk could be found. Abort')
    sys.exit(1)

if accomp_pk in processing_store['processed_accompaniments']:
    print('Accompaniment', accomp_pk, 'is already processed')
    sys.exit(1)

selected_accomp = None
for accomp in piece_detail['accompaniments']:
    if accomp['pk'] == accomp_pk:
        selected_accomp = accomp
        break
if selected_accomp is None:
    print('Could not found accompaniment in piece detail')
    sys.exit(1)

recordings = selected_accomp['recordings']
if not recordings:
    print('No recording found for this accompaniment')
    sys.exit(1)

musicxml_url = selected_accomp['musicxml_file'] or selected_accomp['solo_musicxml_file']
asco_url = selected_accomp['asco_file']
mp3_url = recordings[0]['compressed_file']

assert musicxml_url, 'No musicxml could be found'
assert asco_url, 'No asco could be found'
assert mp3_url, 'No mp3 could be found'

musicxml_file = download_file(musicxml_url)
asco_file = download_file(asco_url)
mp3_file = download_file(mp3_url)

assert musicxml_file, 'No musicxml file could be found'
assert asco_file, 'No asco file could be found'
assert mp3_file, 'No mp3 file could be found'

print('Process video for', piece_pk, accomp_pk)

output_file = '/app/tools/final_output.mp4'
try:
    os.remove(output_file)
except:
    pass
subprocess.run(['videogen.sh', musicxml_file, asco_file, mp3_file, output_file])
if not os.path.exists(output_file):
    print('Error generating video')
    sys.exit(1)
print('Done processing video in', output_file)
# We process the video here
# We upload the video here

processing_store['processed_pieces'].append(piece_pk)
processing_store['processed_accompaniments'].append(accomp_pk)

with open(processing_store_file, 'w') as handle:
    json.dump(processing_store, handle)
