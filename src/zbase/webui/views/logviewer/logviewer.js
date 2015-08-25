ZBase.registerView((function() {

  var query_mgr;
  var logfiles;
  var next_page_time;
  var pagination_history = [];

  var init = function(params) {
    $.showLoader();
    query_mgr = EventSourceHandler();

    $.httpGet("/api/v1/logfiles", function(r) {
      if (r.status == 200) {
        logfiles = JSON.parse(r.response).logfile_definitions;
        render();
        updateQuery(params.path);
      } else {
        $.fatalError();
      }

      $.hideLoader();
    });
  };

  var destroy = function() {
    query_mgr.closeAll();
  };

  var updateQuery = function(url) {
    console.log("update query");
    setQueryParams(url);
    executeQuery();
  }

  var executeQuery = function() {
    showLoadingBar(0);

    var params = getQueryParams();
    console.log(params);

    var url = "/api/v1/logfiles/scan?" + $.buildQueryString(params);
    var query = query_mgr.get("logfile_query", url);

    query.addEventListener("message", function(e) {
      var data = JSON.parse(e.data);
      var result = data.result;

      if (result.length > 0) {
        showLoadingBar(data.rows_scanned);
        renderResult(result);
      }

      if (data.status == "finished") {
        query_mgr.close("logfile_query");
        hideLoadingBar();

        next_page_time = result[result.length - 1].time;
      }
    });

    query.addEventListener("error", function(e) {
      query_mgr.close("logfile_query");
      hideLoadingBar();
      renderError();
    });
  };

  var getQueryParams = function() {
    var params = {};

    // param: logfile
    params.logfile = $(".zbase_logviewer z-dropdown.logfile_control").getValue();

    // param: time
    var time_control = $(".zbase_logviewer .time_control");
    params.time =  time_control.getAttribute("data-timestamp");

    // param: filter
    params.filter_type = $(".zbase_logviewer .filter_type_control").getValue();
    params.filter = $(".zbase_logviewer .filter_control").getValue();

    // param: columns
    params.columns = $(".zbase_logviewer .columns_control").getValue();

    // param: limit
    params.limit = "100";

    return params;
  };

  var setQueryParams = function(url) {
    // param: logfile
    var logfile = UrlUtil.getParamValue(url, "logfile");
    setLogfileParam(logfile);

    // param: end time
    var end_time = UrlUtil.getParamValue(url, "time");
    setTimeParam(end_time);

    // param: filter type
    var filter_type = UrlUtil.getParamValue(url, "filter_type");
    var filter = UrlUtil.getParamValue(url, "filter");
    setFilterParam(filter_type, filter);

    // param: columns
    var columns = UrlUtil.getParamValue(url, "columns");
    setColumnsParam(logfile, columns);

    // param: raw
    var raw = UrlUtil.getParamValue(url, "raw");
  };

  var setLogfileParam = function(logfile) {
    var dropdown = $(".zbase_logviewer z-dropdown.logfile_control");
    var items = $("z-dropdown-items", dropdown);
    items.innerHTML = "";

    logfiles.forEach(function(logfile) {
      var item = document.createElement("z-dropdown-item");
      item.innerHTML = logfile.name;
      item.setAttribute("data-value", logfile.name);
      items.appendChild(item);
    });

    dropdown.setValue([logfile]);
  };

  var setTimeParam = function(end_time) {
    if (typeof end_time == "string") {
      end_time = parseInt(end_time, 10);
    }

    if (!end_time) {
      end_time = (new Date()).getTime() * 1000;
    }

    var time_control = $(".zbase_logviewer .time_control");
    time_control.setAttribute("data-timestamp", end_time);
    time_control.value = DateUtil.printTimestamp(end_time);
  }

  var setFilterParam = function(filter_type, filter) {
    if (!filter_type) {
      filter_type = "sql";
    }

    $(".zbase_logviewer .filter_type_control").setValue([filter_type]);
    $(".zbase_logviewer .filter_control").setValue(filter);
  }

  var setColumnsParam = function(logfile, columns_str) {
    if (!columns_str) {
      columns_str = "__all__";
    }

    var dropdown = $(".zbase_logviewer .columns_control");
    var columns = listLogfileColumns(logfile);
    var items = $("z-dropdown-items", dropdown);
    items.innerHTML = "";

    {
      var item = document.createElement("z-dropdown-item");
      item.innerHTML = "<z-checkbox></z-checkbox> " + "All Columns";
      item.setAttribute("data-mutually-exclusive", "yes");
      item.setAttribute("data-value", "__all__");
      items.appendChild(item);
    }

    columns.forEach(function(column) {
      var item = document.createElement("z-dropdown-item");
      item.innerHTML = "<z-checkbox></z-checkbox> " + column;
      item.setAttribute("data-value", column);
      items.appendChild(item);
    });

    dropdown.setValue(columns_str.split(","));
  }

  var submitControls = function() {
    next_page_time = null;
    pagination_history.clear();

    var params = getQueryParams();
    var url = "/a/logviewer?" + $.buildQueryString(params);
    $.navigateTo(url);
  }

  var goToNextPage = function() {
    if (!next_page_time) {
      return;
    }

    $(".zbase_logviewer .z-pager .prev").removeAttribute("data-disabled");

    var params = getQueryParams();
    params.time = next_page_time;
    var url = "/a/logviewer?" + $.buildQueryString(params);
    pagination_history.push(url);
    $.navigateTo(url);
  };

  var goToPreviousPage = function() {
    var url = pagination_history.pop();
    if (pagination_history.length == 0) {
      $(".zbase_logviewer .z-pager .prev").setAttribute("data-disabled", true);
    }

    if (url) {
      $.navigateTo(url);
    }
  };

  var findLogfileDefinition = function(logfile_name) {
    for (var i = 0; i < logfiles.length; ++i) {
      if (logfiles[i].name == logfile_name) {
        return logfiles[i];
      }
    }

    return null;
  }

  var listLogfileColumns = function(logfile_name) {
    var logfile_def = findLogfileDefinition(logfile_name);
    if (!logfile_def) {
      $.fatalError();
      return null;
    }

    var columns = [];
    for (var i = 0; i < logfile_def.source_fields.length; ++i) {
      columns.push(logfile_def.source_fields[i].name);
    }

    for (var i = 0; i < logfile_def.row_fields.length; ++i) {
      columns.push(logfile_def.row_fields[i].name);
    }

    return columns;
  }

  var render = function() {
    console.log("render");
    var page = $.getTemplate("views/logviewer", "zbase_logviewer_main_tpl");

    $(".logfile_control", page).addEventListener("change", submitControls);
    $(".filter_type_control", page).addEventListener("change", submitControls);
    $(".filter_control", page).addEventListener("change", submitControls);
    $(".columns_control", page).addEventListener("change", submitControls);
    $(".time_control", page).addEventListener("change", submitControls);
    $(".z-pager .next", page).addEventListener("click", goToNextPage);
    $(".z-pager .prev", page).addEventListener("click", goToPreviousPage);

    $.handleLinks(page);
    $.replaceViewport(page);
  };

  var renderResult = function(result) {
    console.log("render result", result);
  }

  var renderError = function(error) {
    console.log("render error", error);
  }

  var showLoadingBar = function(num_rows) {
    $(".zbase_logviewer table").classList.add("loading");
    var elem = $(".zbase_logviewer .loglines_loading_bar");
    elem.classList.remove("hidden");
    $(".loading_bar_info", elem).style.visibility = 
        (num_rows > 0 ? "visible" : "hidden");
    $(".num_rows", elem).innerHTML = num_rows;
  };

  var hideLoadingBar = function() {
    $(".zbase_logviewer table").classList.remove("loading");
    $(".zbase_logviewer .loglines_loading_bar").classList.add("hidden");
  };

  return {
    name: "logviewer",
    loadView: init,
    unloadView: destroy,
    handleNavigationChange: updateQuery
  };

})());


/*
      this.setInitialParams();

      if (this.logfile == null) {
        this.pane = document.querySelector("[name='logviewer_logfile_control']");
        this.hideLoader();
        this.loadLogfiles();
      } else {
        this.pane = document.querySelector("[name='logviewer_pane']");
        this.source_handler = Util.eventSourceHandler();
        this.updateParams();
        this.loadLogfileDescription();
        this.loadLogfiles();
        this.setParamControls();
        this.handleSearch();
        this.handleTimeControl();
        this.handleColumnSelect();
        this.handleUpdateEndtimeControl();

        var _this = this;
        window.addEventListener("popstate", function() {
          _this.setInitialParams();
          _this.updateParams();
          _this.setParamControls();
        }, false);
      }


    this.updateEndtime = function() {
      this.initial_end_time = Date.now() * 1000;
    };

    this.showUpdateEndtimeControl = function() {
      document.getElementById("update_endtime_control")
        .style.display = "inline-block";
    };

    this.handleUpdateEndtimeControl = function() {
      var _this = this;
      var control = document.getElementById("update_endtime_control");
      control.querySelector("a").addEventListener("click", function() {
        _this.updateEndtime();
        history.pushState(
          {param: "until", value: _this.initial_end_time},
          "Until",
          UrlUtil.addOrModifyUrlParam(
            window.location.href,
            "until",
            _this.initial_end_time));
        _this.updateParams();
        control.style.display = "none";
      }, false);
    };

    this.updateParams = function() {
    };

    this.setParamControls = function() {
    };


    this.loadLogfiles = function() {
      var logfiles = [
        {
          name: "access_log"
        },
      ];

      this.renderLogfileSelect(logfiles);
    };

    this.loadLogfileDescription = function() {
      var _this = this;
      var id = "descr";
      var source = this.source_handler.get(
        id,
        "/api/v1/sql_stream?query=" + encodeURIComponent("DESCRIBE 'logs." + this.logfile + "';"));

      source.addEventListener('result', function(e) {
        _this.source_handler.close(id);
        var result = JSON.parse(e.data).results;
        _this.renderColumnSelect(result[0].rows);
      });

      source.addEventListener('error', function(e) {
        _this.source_handler.close(id);
      });
    };


    this.isRawView = function() {
      return this.columns == "__all__";
    };

    this.handlePagination = function() {
      var _this = this;

      function updateUntilParam() {
        var until = _this.page_times[_this.page_times.length - 1];
        history.pushState(
          {"until": until},
          "Until",
          UrlUtil.addOrModifyUrlParam(
            window.location.href,
            "until",
            until));
      };

      document.querySelector("[data-action='show-prev-loglines']")
        .addEventListener("click", function() {
          if (_this.page_times.length < 3) {
            return;
          }
          _this.page_times.pop();
          _this.renderPager();
          _this.loadLogfileData(
              _this.page_times[_this.page_times.length - 1],
              true);
          updateUntilParam();
        }, false);

      document.querySelector("[data-action='show-next-loglines']")
        .addEventListener("click", function() {
          _this.loadLogfileData(
              _this.page_times[_this.page_times.length - 1],
              true);
          _this.showUpdateEndtimeControl();
          updateUntilParam();
        }, false);
    };

    this.renderPager = function() {
      if (this.page_times.length > 2) {
        this.pane.querySelector(".fn-pager-tooltip-back")
          .removeAttribute("data-disabled");
      } else {
        this.pane.querySelector(".fn-pager-tooltip-back")
          .setAttribute("data-disabled", "");
      }
    };


    this.renderRaw = function(rows) {
      var _this = this;
      var tbody = document.querySelector("table[name='loglines'] tbody");
      tbody.innerHTML = "";

      for (var i = 0; i < rows.length; i++) {
        var tr = document.createElement('tr');
        var folded_tr = document.createElement("tr");
        tr.className = "folding";
        folded_tr.className = "folded";

        tr.innerHTML =
            "<td class='fold_icon'><i class='fa'></i></td><td class='time'>" +
            _DateUtil.printTimestamp(rows[i].time) +
            "</td><td><span>" + rows[i].raw + "</span></td>";

        rows[i].columns.forEach(function(column) {
          folded_tr.innerHTML += "<td>" + column + "</td>";
        });

        tbody.appendChild(tr);
        tbody.appendChild(folded_tr);

        tr.addEventListener("click", function() {
          if (this.hasAttribute("data-active")) {
            this.removeAttribute("data-active");
          } else {
            this.setAttribute("data-active", "active");
          }
        }, false);
      }
    };

    this.renderStructuredWithRaw = function(rows) {
      var _this = this;
      var tbody = document.querySelector("table[name='loglines'] tbody");
      tbody.innerHTML = "";

      for (var i = 0; i < rows.length; i++) {
        var tr = document.createElement('tr');
        var folded_tr = document.createElement("tr");
        tr.className = "folding";

        tr.innerHTML =
          "<td class='fold_icon'><i class='fa'></i></td><td class='time'>" +
          _DateUtil.printTimestamp(rows[i].time) +"</td>";

        rows[i].columns.forEach(function(column) {
          tr.innerHTML += "<td><span>" + column + "</span></td>";
        });

        folded_tr.className = "folded";
        folded_tr.innerHTML = 
          "<td colspan='" + (rows[i].columns.length + 2) + "'><span>" +
          rows[i].raw + "</span></td>";
        tbody.appendChild(tr);
        tbody.appendChild(folded_tr);

        tr.addEventListener("click", function() {
          if (this.hasAttribute("data-active")) {
            this.removeAttribute("data-active");
          } else {
            this.setAttribute("data-active", "active");
          }
        }, false);
      }
    };


    this.renderStructured = function(rows) {
      var _this = this;
      var tbody = document.querySelector("table[name='loglines'] tbody");
      tbody.innerHTML = "";

      for (var i = 0; i < rows.length; i++) {
        var tr = document.createElement('tr');
        tr.innerHTML =
            "<td class='time'>" + _DateUtil.printTimestamp(rows[i].time) +"</td>";

        rows[i].columns.forEach(function(column) {
          tr.innerHTML += "<td><span>" + column + "</span></td>";
        });

        tbody.appendChild(tr);
      }
    };




    this.renderColumnSelect = function(columns) {
      var _this = this;
      var dropdown = this.pane.querySelector(
        "z-dropdown[data-action='logfile-columns-select']");
      var items = dropdown.querySelector("z-dropdown-items");
      var selection_check = this.pane.querySelector(
        "[name='column_selection_check']");

      columns.forEach(function(column) {
        if (column[0] == "raw") {return;}
        var item = document.createElement("z-dropdown-item");
        var checkbox = document.createElement("fn-checkbox");
        item.setAttribute("data-value", column[0]);
        item.appendChild(checkbox);
        item.innerHTML += " " + column[0];
        items.appendChild(item);
      });

      this.updateColumnSelectView();
    };

    this.updateRawParam = function(updateView) {
      history.pushState(
        {param: "raw", value: this.raw},
        "raw",
        UrlUtil.addOrModifyUrlParam(
          window.location.href, "raw", this.raw));

      if (updateView) {
        if (this.raw == "false") {
          this.pane.querySelector(
            "[name='column_selection_check'] fn-checkbox")
              .removeAttribute("data-active");
        } else {
          this.pane.querySelector(
            "[name='column_selection_check'] fn-checkbox")
              .setAttribute("data-active", "active");
        }
      }
    };

    this.updateColumnSelectView = function() {
    };

    this.handleColumnSelect = function() {
      var _this = this;
      var dropdown = this.pane.querySelector(
        "z-dropdown[data-action='logfile-columns-select']");
      var selection_check = this.pane.querySelector(
        "[name='column_selection_check']");
      var checkbox = selection_check.querySelector("fn-checkbox");

      checkbox.addEventListener("click", function() {
        _this.raw = this.hasAttribute("data-active");
        _this.updateParams();
        _this.updateRawParam(false);
      }, false);

      dropdown.querySelector("fn-button").addEventListener("click", function() {
        dropdown.hideDropdown();
        _this.columns = dropdown.getValue();
        _this.updateParams();
        history.pushState(
          {param: "columns", value: _this.columns},
          "Columns",
          UrlUtil.addOrModifyUrlParam(
            window.location.href,
            "columns",
            _this.columns));
      }, false);


      dropdown.addEventListener("z-dropdown-item-click", function(e) {
        var target = e.srcElement || e.target;
        var value = target.getAttribute("data-value");

        if (value == "__all__") {
          var selected_items = this.querySelectorAll("[data-selected]");
          for (var i = 0; i < selected_items.length; i++) {
            if (selected_items[i].getAttribute("data-value") != value) {
              selected_items[i].removeAttribute("data-selected");
              selected_items[i].querySelector("fn-checkbox")
                .removeAttribute("data-active");
            }
          }

          if (_this.raw == "false") {
            _this.raw = "true";
            _this.updateRawParam(true);
          }
          selection_check.style.display = "none";
        } else {
          var row = dropdown.querySelector("[data-value='__all__']");
          row.removeAttribute("data-selected");
          row.querySelector("fn-checkbox").removeAttribute("data-active");
          selection_check.style.display = "inline-block";
          if (_this.raw == "true") {
            _this.raw = "false";
            _this.updateRawParam(true);
          }
        }
        dropdown.setHeaderValue();
      }, false);

    };



    this.handleSearch = function() {
      var search = document.querySelector("fn-search[name='logviewer-filter']");
      var input = search.querySelector("input");
      var dropdown = document.querySelector("z-dropdown[name='logviewer-filter']");
      var error_note = document.querySelector(".error_note[name='logviewer-filter']");
      var _this = this;

      dropdown.addEventListener("z-dropdown-item-click", function(e) {
        var target = e.srcElement || e.target;
        input.placeholder = "Filter by " + target.getAttribute("data-text");
        _this.filter_type = target.getAttribute("data-value");
        history.pushState(
          {param: "filter_type", value: input.value},
          "Filter Type",
          UrlUtil.addOrModifyUrlParam(
            window.location.href,
            "filter_type",
            _this.filter_type));
      }, false);

      search.addEventListener("fn-search-submit", function() {
        if (input.value.length == 0) {
          error_note.classList.remove("hidden");
          return;
        }

        _this.filter = encodeURIComponent(input.value);
        _this.updateParams();
        history.pushState(
          {param: "filter", value: input.value},
          "Filter",
          UrlUtil.addOrModifyUrlParam(
            window.location.href,
            "filter",
            encodeURIComponent(input.value)));
      }, false);
    };

    this.updateTimeControl = function(end_time) {
    };


    this.handleTimeControl = function() {
      var _this = this;
      var control_input = document.getElementById("time_control");
      var widget = document.getElementById("logviewer_date_widget");
      var inputs = widget.querySelectorAll("input");
      var apply_button = document.querySelector(
        "fn-button[data-action='set-logviewer-date']");

      control_input.addEventListener("click", function() {
        if (widget.classList.contains("hidden")) {
          widget.classList.remove("hidden");
        } else {
          widget.classList.add("hidden")
        }
      }, false);


      for (var i = 0; i < inputs.length; i++) {
        inputs[i].addEventListener("blur", function() {
          switch (this.value.length) {
            case 0:
              this.value = "00";
              break;

            case 1:
              var value = parseInt(this.value, 10);
              if (isNaN(value)) {
                this.classList.add("error");
                apply_button.setAttribute("data-state", "disabled");
                return;
              }
              this.value = Fnord.appendLeadingZero(value);
              break;

            case 2:
              if (isNaN(parseInt(this.value[0], 10)) ||
                  isNaN(parseInt(this.value[1], 10))) {
                this.classList.add("error");
                apply_button.setAttribute("data-state", "disabled");
                return;
              }
          }

          var value = parseInt(this.value, 10);
          if (this.getAttribute("data-factor") == "3600") {
            if (value > 23) {
              this.classList.add("error");
              apply_button.setAttribute("data-state", "disabled");
              return;
            }
          } else {
            if (value > 59) {
              this.classList.add("error");
              apply_button.setAttribute("data-state", "disabled");
              return;
            }
          }

          this.classList.remove("error");
          if (widget.querySelector("input.error") == null) {
            apply_button.removeAttribute("data-state");
          }
        }, false);
      }

      apply_button.addEventListener("click", function() {
        var timestamp = DateUtil.parseTimestamp(
          parseInt(widget.querySelector("fn-calendar")
            .getAttribute("data-selected"), 10));
        for (var i = 0; i < inputs.length; i++) {
          var value = parseInt(inputs[i].value, 10);
          timestamp += value * parseInt(inputs[i].getAttribute("data-factor"), 10);
        }

        timestamp = timestamp * 1000;
        _this.initial_end_time = timestamp;
        _this.updateParams();
        widget.classList.add("hidden");
        history.pushState(
          {param: "until", value: timestamp},
          "Until",
          UrlUtil.addOrModifyUrlParam(
            window.location.href, "until", timestamp));
      }, false);
    };

  */
