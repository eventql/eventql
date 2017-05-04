'use strict'

const assert = require("assert");
const EventQL = require('../lib/eventql');
const Auth = require('../lib/auth');

const database = 'test';
const port = 9175;
const host = 'localhost';

describe('EventQL', function() {
  describe('constructor', function() {
    it("it should throw an exception if host, port or auth aren't correctly provided", function() {
      assert.throws(function() {
        new EventQL();
      }, Error);

      assert.throws(function() {
        new EventQL(1);
      }, Error);

      assert.throws(function() {
        new EventQL('localhost');
      }, Error);

      assert.throws(function() {
        new EventQL('localhost', 9175);
      }, Error);
    });
  });

  describe('createTable', function() {
    it("should create a new table", function(done) {
      const eventql = new EventQL(host, port, new Auth(database));
      eventql.createTable('sensors1', [
        {
          id: 1,
          name: 'time',
          type: 'DATETIME',
          optional: false,
          repeated: false
        },
        {
          id: 2,
          name: 'name',
          type: 'STRING',
          optional: false,
          repeated: false
        },
        {
          id: 3,
          name: 'value',
          type: 'STRING',
          optional: false,
          repeated: false
        }
      ], (error) => {
        if (error !== undefined) {
          assert.equal('error: table already exists', error.message);
        }
        done();
      })
    });
  });

  describe('insert', function() {
    it("should insert a new record", function(done) {
      const eventql = new EventQL(host, port, new Auth(database));
      const time = new Date().toISOString()

      eventql.insert('sensors', {
        name: 'name',
        value: 'value',
        time: time
      }, (error) => {
        assert.equal(undefined, error);
        done();
      })
    });
  });

  describe('execute', function() {
    it("should execute an sql query and return the result", function(done) {
      const eventql = new EventQL(host, port, new Auth(database));
      eventql.execute('select 1;', {
        onResult: function(results) {
          assert.equal(
              '{"results": [{"type": "table","columns": ["1"],"rows": [["1"]]}]}',
              results);
          done();
        }
      });
    });
  });
});
