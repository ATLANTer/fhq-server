#!/usr/bin/env python
# -*- coding: utf-8 -*-

import libfhqcli
import fhqtest
import sys, traceback
import base64
from pprint import pprint

test_name = 'Testing Public Events'

try:
    fhqtest.print_header(" > > > " + test_name + ": begin ");
    fhqtest.init_enviroment();
    
    r = fhqtest.admin_session.publiceventslist({"page": 0, "onpage": 10});
    if r['count'] > 0:
        pass
        # TODO remove
    
    event_message = fhqtest.generate_random(50).strip()
    event_type = "info"

    r_pub_event1 = fhqtest.admin_session.createpublicevent({
        "type": event_type,
        "message": event_message
    });
    fhqtest.alert(r_pub_event1 == None, 'Could not get response (createpublicevent)')
    fhqtest.check_response(r_pub_event1, "Public Event succesfull created")
    pprint(r_pub_event1)

    # TODO redesign on previus request must return data
    r = fhqtest.admin_session.publiceventslist({"page": 0, "onpage": 10});
    pub_event1 = r['data'][0]
    
    fhqtest.check_values("type of public event", pub_event1['type'], event_type)
    fhqtest.check_values("message of public event", pub_event1['message'], event_message)

    r_event1 = fhqtest.admin_session.getpublicevent({"eventid": pub_event1['id']});
    fhqtest.alert(r_event1 == None, 'Could not get response (getpublicevent)')
    fhqtest.check_response(r_event1, "Public Event succesfull got")
    r_event1 = r_event1['data']
    fhqtest.check_values("(2) type of public event", r_event1['type'], event_type)
    fhqtest.check_values("(2) message of public event", r_event1['message'], event_message)

    r_event1_removed = fhqtest.admin_session.deletepublicevent({"eventid": pub_event1['id']});
    fhqtest.alert(r_event1_removed == None, 'Could not get response (deletepublicevent)')
    fhqtest.check_response(r_event1_removed, "Public Event succesfull removed")

except Exception as e:
    fhqtest.log_err(str(e));
    traceback.print_exc(file=sys.stdout)
finally:
    fhqtest.print_header(" < < < " + test_name + ": end ");
    fhqtest.deinit_enviroment();
