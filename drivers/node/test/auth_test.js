'use strict'

var assert = require('assert');
var Auth = require('../lib/auth');

describe('Auth', function() {
  describe("get", function() {
    it("it should throw an exception if no database was provided", function() {
      assert.throws(function() {
        new Auth();
      }, Error);
    });
  });

  describe("get", function() {
    it("it should return null if no value was set", function() {
      var auth = new Auth('test');
      assert.equal(auth.getUser(), null);
      assert.equal(auth.getAuthToken(), null);
    });
  });

  describe("{set,get}", function() {
    it("it should return a value if set was called", function() {
      var auth = new Auth('my_db');
      auth.setUser('test-user');
      assert.equal(auth.getDatabase(), 'my_db');
      assert.equal(auth.getUser(), 'test-user');
      assert.equal(auth.getAuthToken(), null);
    });
  });
});

