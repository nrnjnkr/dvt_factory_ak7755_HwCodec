var express = require('express');
var router = express.Router();

var fs = require('fs'),
  path = require('path'),
  util = require('util'),
  formidable = require('formidable'),
  config = require('../config'),
  defs = require('../defs'),
  db = require('../db');

const MEDIA_TYPE = defs.MEDIA_TYPE;

/* GET home page. */
// router.get('/', function(req, res, next) {
//   res.render('index', { title: 'Express' });
// });

router.get('/player', function(req, res, next) {
  res.render('player.ejs', { title: 'Players', list:config.player.list });
});

router.get('/rtsp', function(req, res, next) {
  res.render('player_rtsp.ejs', { title: 'RTSP player' });
});

router.get('/upload', function(req, res, next) {
    res.render('upload', { title: 'Upload' });
});

router.post('/', function(req, res, next) {
    handleUpload(req, res);
});

router.post('/upload', function(req, res, next) {
    handleUpload(req, res);
});

const publicPath = config.publicPath;
      uploadPath = path.join(publicPath, config.device.uploadDir),
      rootDir = config.device.rootDir,
      rootPath = path.join(publicPath, rootDir);

function handleUpload(req, res) {
    var form = new formidable.IncomingForm();
    form.uploadDir = uploadPath;
    form.keepExtensions = true;

    form.parse(req, function(err, fields, files) {
      // util.log(util.inspect({fields: fields, files: files}));

      try {
          if (files.file.size > 0) {
            res.send('upload success');
          } else {
            res.send('upload failure');
          }

          var deviceUID = fields['device_id'];
          var eventType = parseInt(fields['event_type']);
          var eventName = fields['event_id'];
          var eventNum = parseInt(fields['event_num']);
          var eventDetail = fields['event_detail'];

          util.log(util.format('#### deviceUID:%s eventType:%d eventName:%s eventNum:%d eventDetail:%s', deviceUID, eventType, eventName, eventNum, eventDetail));

		      var deviceUIDPath = deviceUID;
		      if (config.platform === 'win32' || config.platform === 'win64') {
			       deviceUIDPath = deviceUIDPath.replace(/:/g, '_');
		      }

          var devicePath = path.join(rootPath, deviceUIDPath);
          if (!fs.existsSync(devicePath)) {
              fs.mkdirSync(devicePath);
          }

          if (files.file.size > 0) {
              var typeDir = undefined;
              switch (eventType) {
              case MEDIA_TYPE.VIDEO:      typeDir = config.device.videoDir; break;
              case MEDIA_TYPE.THUMBNAIL:  typeDir = config.device.thumbailDir; break;
              default:                    typeDir = config.device.defaultDir; break;
          }

          var typePath = path.join(devicePath, typeDir);
          if (!fs.existsSync(typePath)) {
              fs.mkdirSync(typePath);
          }

          var eventRelUrl = util.format('/%s/%s/%s', deviceUIDPath, typeDir, files.file.name);
          // util.log(util.format('####%s eventRelUrl:%s url:%s', deviceUIDPath, eventRelUrl, path.join(rootPath, eventRelUrl)));
          fs.renameSync(files.file.path, path.join(rootPath, eventRelUrl));

          if (deviceUID === undefined || eventName === undefined || eventType === undefined || eventNum === undefined || eventRelUrl === undefined) {
              util.log("exit");
          } else {
              db.mediaInsert({
                  uid         : deviceUID,
                  eventName   : eventName,
                  eventType   : eventType,
                  eventNum    : eventNum,
                  eventRelUrl : eventRelUrl
              },
              function(err) {
                  if (err) { util.log('mediaInsert completed - ' + err) };
              });
            }
          } else {
              fs.unlinkSync(filePath);
          }
      } catch (err) {
            util.log(files);
        }
    });
}

module.exports = router;