
INCLUDEPATH += src/tasks/headers
INCLUDEPATH += src/tasks

HEADERS += \
	src/tasks/headers/add_public_events_task.h \
	src/tasks/headers/update_max_score_game_task.h \
        src/tasks/headers/update_quest_solved_task.h \
        src/tasks/headers/update_user_location_task.h \
        src/tasks/headers/update_user_rating_task.h \
	src/tasks/runtasks.h

SOURCES += \
	src/tasks/impl/add_public_events_task.cpp \
	src/tasks/impl/update_max_score_game_task.cpp \
        src/tasks/impl/update_quest_solved_task.cpp \
        src/tasks/impl/update_user_rating_task.cpp \
        src/tasks/impl/update_user_location_task.cpp \
	src/tasks/runtasks.cpp \

	

