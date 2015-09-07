var SQLEditorResultList = function(results) {

  var renderResultTable = function(elem, rows, columns) {
    var table = document.createElement("table");
    table.className = 'z-table';

    innerHTML = "<thead><tr>";
    columns.forEach(function(column) {
      innerHTML += "<th>" + $.escapeHTML(column) + "</th>";
    });

    innerHTML += "</tr></thead><tbody>";

    rows.forEach(function(row) {
      innerHTML += "<tr>";
      row.forEach(function(cell) {
        innerHTML += "<td>" + $.escapeHTML(cell) + "</td>";
      });
      innerHTML += "</tr>";
    });

    innerHTML += "</tbody>";
    table.innerHTML = innerHTML;
    elem.appendChild(table);
  };

  var renderResultChart = function(elem, svg) {
    var chart = document.createElement("div");
    chart.className = "zbase_sql_chart";
    chart.innerHTML = svg;
    elem.appendChild(chart);
  };

  var renderResultBar = function(elem, result_index) {
    var bar = document.createElement("div");
    bar.className = "zbase_result_pane_bar";
    bar.setAttribute('data-active', 'active');
    bar.innerHTML ="<label>Result " + (result_index + 1) + "</label>";

    if (results.length > 1) {
      bar.className += " clickable";
      bar.addEventListener('click', function() {
        if (this.hasAttribute('data-active')) {
          this.removeAttribute('data-active');
        } else {
          this.setAttribute('data-active', 'active');
        }
      }, false);
    }

    elem.appendChild(bar);
  };

  var render = function(elem) {
    elem.innerHTML = "";

    for (var i = 0; i < results.length; i++) {
      renderResultBar(elem, i);

      switch (results[i].type) {
        case "chart":
          renderResultChart(elem, results[i].svg);
          break;
        case "table":
          renderResultTable(elem, results[i].rows, results[i].columns)
          break;
      }
    }
  };

  return {
    render: render
  };

};

