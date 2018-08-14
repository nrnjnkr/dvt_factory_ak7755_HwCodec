var express = require('express');
var path = require('path');
var favicon = require('serve-favicon');
var logger = require('morgan');
var cookieParser = require('cookie-parser');
var bodyParser = require('body-parser');

var routes = require('./routes/index');
var api = require('./routes/api');
var api_v1 = require('./routes/api_v1');
var fs = require('fs');
var url = require('url');
var config = require('./config');
const devicesPath = path.join(config.publicPath, config.device.rootDir);

var app = express();

// view engine setup
app.set('views', path.join(__dirname, 'views'));
app.set('view engine', 'ejs');

// uncomment after placing your favicon in /public
//app.use(favicon(__dirname + '/public/favicon.ico'));
app.use(logger('dev'));
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: false }));
app.use(cookieParser());
app.use(express.static(path.join(__dirname, 'public'), { dotfiles: 'allow' }));
app.use(express.static(devicesPath));

app.use('/', routes);
app.use('/api', api);
app.use('/api/v1', api_v1);

// catch 404 and forward to error handler
app.use(function(req, res, next) {
  var local_url = url.parse(req.url, true);
  var pathname = local_url.pathname.replace(/%20/g,' '),
      re=/(%[0-9A-Fa-f]{2}){3}/g;
  pathname = pathname.replace(re, function(word){
      var buffer = new Buffer(3),
      array = word.split('%');
      array.splice(0,1);
      array.forEach(function(val,index) {
        buffer[index]=parseInt('0x'+val,16);
      });
      return buffer.toString('utf8');
  });

  var filename = path.join(devicesPath, pathname);
  fs.exists(filename, function(exists) {
    if (!exists) {
        var err = new Error('Not Found');
        err.status = 404;
        next(err);
    } else {
      fs.stat(filename, function(err, stat) {
        if (stat.isDirectory()) {
          listDirectory(filename, req, res);
        }
      });
    }
  });
});

// error handlers

// development error handler
// will print stacktrace
if (app.get('env') === 'development') {
  app.use(function(err, req, res, next) {
    console.log('development');
    res.status(err.status || 500);
    res.render('error', {
      message: err.message,
      error: err
    });
  });
}

// production error handler
// no stacktraces leaked to user
app.use(function(err, req, res, next) {
  console.log('production');
  res.status(err.status || 500);
  res.render('error', {
    message: err.message,
    error: {}
  });
});


function listDirectory(parentDirectory, req, res) {
  fs.readdir(parentDirectory, function(error, files) {
    var list = [];
    files.forEach(function(val, index) {
      var stat = fs.statSync(path.join(parentDirectory, val));
      if (stat.isDirectory(val)) {
        val = path.basename(val)+"/";
      } else {
        val = path.basename(val);
      }
      list[index] = { name: val, time:stat.ctime, size: stat.size};
    });
    res.render('filelist', { title: config.version, apiList:config.api.list, fileList: list});
  });
}

module.exports = app;
