##############################################################################
##
##       Copyright (C) 2018 Frank Eskesen.
##
##       This file is free content, distributed under the MIT license.
##       (See accompanying file LICENSE.MIT or the original contained
##       within https://opensource.org/licenses/MIT)
##
##############################################################################
##
## Filename-
##       newsreader.pro
##
## Purpose-
##       NewsReader properties file.
##
## Last change date-
##       2018/01/01
##
##############################################################################

##############################################################################
# Property: ControlServlet.debug
# Property: MainLogger::debug
#  Content: Debugging verbosity
#  Default: 0
debug=2

##############################################################################
# Property: ControlServlet.verbose
# Property: MainLogger.verbose
#  Content: Debugging verbosity
#  Default: 0
verbose=2

##############################################################################
# Property: NewsReader.database-path
#  Content: The path for all NewsReader databases
#  Default: .
database-path=/home/data/web/database/newsreader

##############################################################################
# Property: NewsReader.max-article-count
#  Content: The maximum number of articles to be read
#  Default: 0 (no limit)
max-article-count=500

##############################################################################
# Property: NewsReader.news-groups-file
#  Content: The name of the news group database
#  Default: groups.all
news-groups-file=groups.all

##############################################################################
# Property: NewsReader.news-server-file
#  Content: The name of the news server database
#  Default: article.db
news-server-file=article.db

##############################################################################
# Property: NewsReader.subs-groups-file
#  Content: The name of the news group subscription database
#  Default: groups.sub
subs-groups-file=groups.sub

##############################################################################
# Property: NewsReader::subs-groups-zero
#  Content: Boolean: Should the subgroups be reset?
#  Default: false
# subs-groups-zero=true

##############################################################################
# Property: NewsReader.news-server-name
#  Content: The name of the news server
#  Default: 127.0.0.1 (Effectively disabling it)
#
# Property: NewsReader.news-server-port
#  Content: The port of the news server
#  Default: 119
news-server-name=nntp.aioe.org
news-server-name=news.task.gda.pl
news-server-name=127.0.0.1

news-server-port=119
news-server-port=65025

##############################################################################
# Property: NewsReader.refresh-interval
#  Content: The number of seconds between database updates
#  Default: 1800 (30 minutes)
refresh-interval=30
refresh-interval=1800

##############################################################################
# Property: MainLogger.logfile         NOTE: MainReader/MainLogger property
#  Content: Logfile name
#  Default: logfile.out
# logfile=logfile.out

##############################################################################
# Property: MainReader.runtime         NOTE: MainReader/MainLogger property
#  Content: The number of seconds to operate the MainReader
#  Default: 30
runtime=600

