#!/usr/bin/env python
# -*- coding: utf-8 -*-

import libfhqcli
import fhqtest
import sys, traceback
import base64

try:
    fhqtest.print_header(" > > > Testing Games: begin ");
    fhqtest.init_enviroment();
    game_uuid1 = "00000000-0000-0000-0000-000000000001";

    # Cleanup
    fhqtest.print_bold("Clean up test data... ")
    game1 = fhqtest.admin_session.game_info({ "uuid": game_uuid1 });
    fhqtest.alert(game1 == None, 'Could not get response');
    if game1['result'] == "DONE":
        fhqtest.admin_session.game_delete({ "uuid": game_uuid1, "admin_password": fhqtest.admin_password})
    fhqtest.print_success("Cleaned")

    # test create game
    fhqtest.print_bold("Try create game...")
    game1 = fhqtest.admin_session.game_create({
        "uuid": game_uuid1,
        "name": "game1",
        "description": "game_description",
        "state": "", # TODO not has list of states
        "form": "", # TODO not has list of form
        "type": "online", # TODO not has list of type
        "date_start": "2000-01-01 00:00:00",
        "date_stop": "2001-01-01 00:00:00",
        "date_restart": "2011-01-01 00:00:00",
        "organizators": "team1, team2"
    })
    fhqtest.alert(game1 == None, 'Could not get response (2)');
    fhqtest.check_response(game1, "Game succesfull created")

    # test find game
    fhqtest.print_bold("Find game by uuid... ")
    game1 = fhqtest.admin_session.game_info({ "uuid": game_uuid1 });
    fhqtest.alert(game1 == None, 'Could not get response');
    fhqtest.check_response(game1, "Game found")

    fhqtest.print_bold("Check name... ")
    game1 = game1['data']
    if game1['title'] != "game1": # wrong  param name must be name
        fhqtest.log_err("Game has wrong name");
    else:
        fhqtest.print_success("Game name ok");
    
    gameid = game1['id']
    print("gameid: " + str(gameid))

    # list of games
    fhqtest.print_bold("Get list of games... ")
    games_list = fhqtest.admin_session.games({});
    fhqtest.alert(game1 == None, 'Could not get response');
    fhqtest.check_response(games_list, "Games list got")
    bFound = False
    for game_ in games_list['data']:
        if game_['uuid'] == game_uuid1:
            bFound = True

    if bFound != True:
        fhqtest.log_err("Game not found in list");
    else:
        fhqtest.print_success("Game found in list");

    # test update game name
    fhqtest.print_bold("Update game... ")
    games_list = fhqtest.admin_session.game_update({
        "uuid": game_uuid1,
        "title": "game2",
    });
    game1 = fhqtest.admin_session.game_info({ "uuid": game_uuid1 });
    fhqtest.print_bold("Check name after update... ")
    game1 = game1['data']
    if game1['title'] != "game2": # wrong  param name must be name
        fhqtest.log_err("Game has wrong name");
    else:
        fhqtest.print_success("Game name ok");
    

    # TODO upload log for test game
    f = open("files/game1.png", 'r')
    image_png_base64 = f.read()
    f.close()
    image_png_base64 = base64.b64encode(image_png_base64);
    game_img = fhqtest.admin_session.game_update_logo({
        # "uuid": game_uuid1 # wrong must be uuid
        "gameid": gameid,
        "image_png_base64": image_png_base64
    });
    fhqtest.alert(game_img == None, 'Could not get response');
    fhqtest.check_response(game_img, "Game Logo updated")

    # export
    game_exp = fhqtest.admin_session.game_export({"uuid": game_uuid1});
    fhqtest.alert(game_exp == None, 'Could not get response');
    fhqtest.check_response(game_exp, "Game exported")
    game_zip = base64.b64decode(game_exp['data']['zipfile_base64'])
    f = open(game_exp['data']['zipfile_name'], 'w')
    f.write(game_zip)
    f.close()
    # print(game_exp)

    # remove game
    fhqtest.print_bold("Remove game by uuid... ")
    resp = fhqtest.admin_session.game_delete({ "uuid": game_uuid1, "admin_password": fhqtest.admin_password })
    fhqtest.check_response(resp, "Game removed")
  
    # TODO import
    fhqtest.print_bold("Test game import... ")
    # game1 = fhqtest.admin_session.game_import({ "uuid": game_uuid1 });
    fhqtest.log_warn("Not implemneted yet");
    

    # TODO remove again
    fhqtest.print_bold("Remove game by uuid... ")
    resp = fhqtest.admin_session.game_delete({ "uuid": game_uuid1, "admin_password": fhqtest.admin_password })
    fhqtest.check_response(resp, "Game removed")

except Exception as e:
    fhqtest.log_err(str(e));
    traceback.print_exc(file=sys.stdout)
finally:
    fhqtest.print_header(" < < < Testing Leaks: end ");
    fhqtest.deinit_enviroment();
