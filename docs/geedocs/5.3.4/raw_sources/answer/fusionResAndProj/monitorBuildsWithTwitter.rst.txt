|Google logo|

===========================
Monitor builds with Twitter
===========================

.. container::

   .. container:: content

      If you want to track the progress of a Fusion build when you are
      away from your computer, you can monitor it on Twitter. For an
      example, see `http://twitter.com/keyholefusion <http://twitter.com/keyholefusion>`_. Monitoring Fusion
      builds with Twitter is useful when you are out of the office and
      do not want to SSH into your server and check your builds manually.

      In addition, you can modify this script so you can use it for
      email or other communication systems. Most Linux systems have a
      mail command for sending email, so you can simply replace the curl
      commands with the appropriate commands.

      To set up Fusion for Twitter, you will need:

      -  An Internet connection for your Fusion server.
      -  A Twitter account for your Fusion server.
      -  The cURL tool. (For the Ubuntu ``sudo apt-get install curl``
         command.)
      -  A cron job that is set to call the custom script below.

      .. rubric:: To set up Fusion for Twitter:

      #. Create a new file called ``getwitter``.
      #. Save the file in a directory like ``/usr/bin/`` so it is
         automatically in your path.
      #. Run the ``chmod`` command on the file to allow execution.
      #. Copy the code below to your ``getwitter`` file. Change the
         variables to reflect your Fusion server's Twitter account
         credentials and, if needed, your server's asset root.
         
         .. code-block:: none
         
            :literal:`#!/bin/bash
            
            twittername="YourServersTwitterAccount"
            twitterpass="YourServersTwitterPassword"
            assetroot="/gevol/assets/"
            
            find $assetroot.state -iname "*.task" -ls >> /tmp/filelist
            
            taskcount=0
            inprogresscount=1
            declare -a inprogressarray
            declare -a queuedarray
            
            while read line
            do
            asset=`echo $line | awk '{print $13}'`
            status=`/opt/google/bin/gequery --status $asset`
               if [ "$status" = "InProgress" ]; then
                  inprogressarray[$[${#inprogressarray[@]}+1]]=$asset
               fi
               if [ "$status" = "Queued" ]; then
                  queuedarray[$[${#queuedarray[@]}+1]]=$asset
               fi
            let taskcount=taskcount+1
            done </tmp/filelist
            
            for inprogress in ${inprogressarray[@]}
            do
               logfile=`/opt/google/bin/gequery --logfile $inprogress`
                  while read line
                  do
                  progress=$line
                  done <$logfile
                  
               curl --basic --user "$twittername:$twitterpass" --data-ascii
               "status=#GEEFusion Task$inprogresscount:
               $progress&source=Google+Earth+Enterprise"
               "http://twitter.com/statuses/update.json"
               
               curl --basic --user "$twittername:$twitterpass" --data-ascii
               "status=#GEEFusion Task$inprogresscount:
               $inprogress&source=Google+Earth+Enterprise"
               "http://twitter.com/statuses/update.json"
               
               let inprogresscount=inprogresscount+1
               done
               
               curl --basic --user "$twittername:$twitterpass" --data-ascii
               "status=#GEEFusion Working on ${#inprogressarray[@]} task(s),
               ${#queuedarray[@]} queued &source=Google+Earth+Enterprise"
               "http://twitter.com/statuses/update.json"
               
               rm /tmp/filelist

      #. Add this file to your cron tab and set it to go off every half
         hour or every hour.

         ``# crontab -e``

      #. To run the Twitter update every 30 minutes, add the line:

         ``*/30 * * * * /usr/bin/getwitter``

      #. Follow your Fusion server on Twitter.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
