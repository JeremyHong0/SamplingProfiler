var fileName = "data/ProfileReport.csv";

// set the dimensions and margins of the graph
const margin = {top: 150, right: 10, bottom: 250, left: 80},
    width = 1000 - margin.left - margin.right,
    height = 800 - margin.top - margin.bottom;

// append the svg object to the body of the page
const svg = d3.select("#chart-area")
  .append("svg")
    .attr("width", width + margin.left + margin.right)
    .attr("height", height + margin.top + margin.bottom)
  .append("g")
    .attr("transform", `translate(${margin.left}, ${margin.top})`);

// Parse the Data
d3.csv(fileName).then (function(data) {

  // sort data
  data.sort(function(b, a) {
    return +a.HitCount - +b.HitCount;
  }).slice(0, 10);
  console.log(data);

  data.filter(function(d,i){ return i > 10 });

  // X axis
  const x = d3.scaleBand()
    .range([ 0, width ])
    .domain(data.map(d => d.Function))
    .padding(0.2);
  svg.append("g")
    .attr("transform", `translate(0, ${height})`)
    .call(d3.axisBottom(x))
    .selectAll("text")
      .attr("transform", "translate(-10,0)rotate(-45)")
      .style("text-anchor", "end");

  // Add Y axis
  var domainY = data[0].HitCount;
  //console.log(domainY);

  const y = d3.scaleLinear()
    .domain([0, domainY])
    .range([ height, 0]);
  svg.append("g")
    .call(d3.axisLeft(y));

  // Bars
  svg.selectAll("mybar")
    .data(data)
    .enter()
    .append("rect")
      .attr("x", d => x(d.Function))
      .attr("y", d => y(d.HitCount))
      .attr("width", x.bandwidth())
      .attr("height", d => height - y(d.HitCount))
      .attr("fill", "#69b3a2")

})