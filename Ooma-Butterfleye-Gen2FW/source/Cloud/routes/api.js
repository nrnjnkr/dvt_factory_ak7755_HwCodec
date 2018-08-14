var express = require('express');
var router = express.Router();

var fs = require('fs'),
	path = require('path'),
	util = require('util'),
	config = require('../config'),
	db = require('../db');

/* GET apis listing. */
router.get('/', function(req, res, next) {
	res.render('api.ejs', { title: 'APIs', list:config.api.list });
});

module.exports = router;