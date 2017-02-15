var SensorChart = function(canvas, chartLabel, xLabel, yLabel) {
    var itsCanvas = canvas;
    var _this = this;
    var data = {
        labels: [],
        datasets: [
            {
                label: chartLabel,
                fill: false,
                lineTension: 0.1,
                backgroundColor: "rgba(255,255,255,0.8)",
                borderColor: "rgba(0,0,0,1)",
                borderCapStyle: 'butt',
                borderDash: [],
                borderDashOffset: 0.0,
                borderJoinStyle: 'miter',
                pointBorderColor: "rgba(0,0,0,1)",
                pointBackgroundColor: "#fff",
                pointBorderWidth: 1,
                pointHoverRadius: 5,
                pointHoverBackgroundColor: "rgba(75,192,192,1)",
                pointHoverBorderColor: "rgba(220,220,220,1)",
                pointHoverBorderWidth: 2,
                pointRadius: 5,
                pointHitRadius: 10,
                data: [],
            }
        ]
    };
    var option = {
	    showLines: true,
        scales: {
            yAxes: [{
		    scaleLabel: {
			display: true,
			labelString: yLabel
		    }
            }],
            xAxes: [{
		scaleLabel: {
			display: true,
			labelString: xLabel
		}
            }]
        }
    };
    var itsChart = Chart.Line(canvas,{
        data: data,
        options: option
    });

    this.addChartData = function(value, label){
        var newIndex = itsChart.data.datasets[0].data.length;
		var newLabel = label;
        itsChart.data.datasets[0].data[newIndex + 1] = value;
		if(!(newIndex % 4)) {
			newLabel = "";
		}
		itsChart.data.labels[newIndex] = newLabel;
        itsChart.update();
    }
}
