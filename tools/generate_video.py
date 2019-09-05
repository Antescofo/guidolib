#! /usr/bin/python3

# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import subprocess
import re
import sys
import os
import json
import requests

VIDEO_GENERATOR_VERSION = 2

SEMI_SUPERVISED = False
DEBUG = True
DEBUG_UPLOAD = False
DEBUG_GENERATE = True
UPLOAD_VIDEO = (not DEBUG) or DEBUG_UPLOAD
GENERATE_VIDEO = ((not DEBUG) and UPLOAD_VIDEO) or DEBUG_GENERATE

BLACKLIST_PIECE_PK = []
def info_to_branch_item(video_title, piece_id):
    item = {
          "type": 2,
          "data": {
                "$marketing_title": video_title,
                "~campaign": "video_generator",
                "~feature": "marketing",
                "~channel": "youtube",
                # "~campaign": "catalogue en ligne",
                # "~feature": "marketing",
                # "~channel": "website",
                "pieceID": str(piece_id),
                "$one_time_use": "false"
                # not necessary for now…
                # "$og_title": f"{composer} - {piece_title}",
                # "$og_description":"",
                # "$og_image_url": ""
            }
        }
    return item


def generate_piece_link(video_title, piece_id):
    BRANCH_BULK_URL = 'https://api.branch.io/v1/url/bulk'
    BRANCH_KEY = 'key_live_ljuUpe9CBveYY3FvP2O1agcmtxjUeuFN'

    if DEBUG:
        BRANCH_KEY = 'key_test_cjxOmmYxusb419wyK9L3phamBBmMdqxW'
    branch_create_bulk_url = BRANCH_BULK_URL + '/' + BRANCH_KEY

    branch_data = [info_to_branch_item(video_title, piece_id)]

    answer = requests.request("POST", branch_create_bulk_url, data=json.dumps(branch_data))
    if answer.status_code != requests.codes.ok:
        raise Exception('Error fetching branch urls')
    print(answer.json())
    return answer.json()[0]['url']

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
    with open(v2_all_store, 'w', encoding='utf-8') as handle:
        handle.write(r.text)

# Load local v2 all
v2_all = None
with open(v2_all_store, 'r', encoding='utf-8') as handle:
    v2_all = json.load(handle)

processing_store = {'processed_pieces': [], 'processed_accompaniments': [], 'accompaniments': {}}

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

instrument_map = {}
for instrument in v2_all['instrument']:
    instrument_map[instrument['pk']] = instrument['title']

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
        if int(piece['pk']) in BLACKLIST_PIECE_PK:
            continue
        piece_pk = piece['pk']
        break

# We can remove this security later
if not DEBUG:
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
    with open(cache_path, 'r', encoding='utf-8') as handle:
        piece_detail = json.load(handle)
else:
    r = requests.get(url_api_piece + str(piece_pk), headers={'Authorization': b'Basic YXBwOm1ldHJvbmF1dDE5MjU='})
    if r.status_code != 200:
        print('Error while fetching piece', r.status_code)
        sys.exit(1)
    with open(cache_path, 'w', encoding='utf-8') as handle:
        handle.write(r.text)
    piece_detail = r.json()

if accomp_pk is None:
    for accomp in piece_detail['accompaniments']:
        accomp_pk = accomp['pk']
        break

if not accomp_pk:
    print('No accomp_pk could be found. Abort')
    sys.exit(1)
if not DEBUG:
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

# Load opus_detail
# Load author_detail



opus_pk = piece_detail['opus']
opus_detail = None
for opus in v2_all['opus']:
    if opus['pk'] == opus_pk:
        opus_detail = opus
        break

author_pk = opus_detail['authors'][0]
author_detail = None
for author in v2_all['author']:
    if author['pk'] == author_pk:
        author_detail = author
        break
piece_title = opus_detail['full_title'] + ' - ' + piece_detail['full_title']
author_title = author_detail['first_name'] + ' ' + author_detail['last_name']
video_title = author_title + ' - ' + piece_title
keywords = [piece_title, author_title, 'sheet music', 'accompaniment', 'metronaut', 'antescofo', 'play along', 'app']

instruments_pk = selected_accomp.get('instruments', [])
if instruments_pk:
    instruments = []
    for instrument_pk in instruments_pk:
        if (instrument_pk in instrument_map) and (instrument_map[instrument_pk].lower() not in video_title.lower()):
            instruments.append(instrument_map[instrument_pk])
    if instruments:
        keywords += instruments
        video_title = ', '.join(instruments) + ' - ' + video_title
video_keywords = ','.join(keywords)
video_category = "10"  # Music, see https://gist.github.com/dgp/1b24bf2961521bd75d6c
video_privacy = "public"
if SEMI_SUPERVISED:
    video_privacy = "unlisted"  # We Check the quality before putting it online
if DEBUG:
    video_privacy = "unlisted"  # For testing only
video_file = None
print(video_title)
print(video_keywords)

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
if GENERATE_VIDEO:
    try:
        os.remove(output_file)
    except:
        pass
    subprocess.run(['videogen.sh', musicxml_file, asco_file, mp3_file, output_file])
    if not os.path.exists(output_file):
        print('Error generating video')
        sys.exit(1)
    print('Done processing video in', output_file)

video_file = output_file
if UPLOAD_VIDEO:
    deep_link_url = generate_piece_link(piece_pk, video_title)
    print('generate piece link:', deep_link_url)
    website_link = "https://www.antescofo.com"
    facebook_link = "https://www.facebook.com/Metronautapp/"
    video_description = """Play {} with accompaniment on Metronaut app: {}\n
Discover Metronaut, the tailor made musical accompaniment app for classical musicians and play masterpieces the way they were meant to be played: with professional musicians to accompany you.\n
Enjoy our growing catalog of music sheets and accompaniments for every instrument and level. Metronaut's accompanists are among the best orchestras and pianists and each accompaniment offers fully acoustic and high quality studio recordings.\n
You're in full control of the digital music sheet: Play or sing hard or previously inaccessible pieces by choosing the speed of the accompaniment and discover pieces not written for your instrument thanks to automatic and quality preserving transposition.\n
Personalize your performance using our speed adaptation feature. Get empowered to play at your own rhythm throughout the piece: Metronaut adapts accompaniment tempo to your interpretation in real-time.\n
Download the App for free: {}
{}
{}""".format(video_title, deep_link_url, deep_link_url, website_link, facebook_link)
    print(video_description)
    print('Uploading video')
    # We upload the video here
    complete_process = subprocess.run(['python3', './upload_video.py',
                                       '--title=' + video_title,
                                       '--description=' + video_description ,
                                       '--keywords=' + video_keywords,
                                       '--category=' + video_category,
                                       '--privacyStatus=' + video_privacy,
                                       '--file=' + video_file],
                                      stdout=subprocess.PIPE)
    if complete_process.returncode != 0:
        print('Something went wrong during the upload')
        sys.exit(1)
    stdout = complete_process.stdout
    match = re.match(b".*Video id '(.*)'.*.*", stdout.replace(b"\n", b" "))
    if not match:
        print('No video id could be found')
        sys.exit(1)
    groups = match.groups()
    if not groups:
        print('No video id could be found 2')
        sys.exit(1)
    video_id = groups[0].decode('ascii')
    print('VIDEO ID:', video_id)
    if not DEBUG:
        print('Updating store')
        processing_store['processed_pieces'].append(piece_pk)
        processing_store['processed_accompaniments'].append(accomp_pk)
        processing_store['accompaniments'][accomp_pk] = {"version": VIDEO_GENERATOR_VERSION, "video_id": video_id}
        with open(processing_store_file, 'w') as handle:
            json.dump(processing_store, handle)
