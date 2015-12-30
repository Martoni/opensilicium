
module top(input clk,
    output D1,
    output D2,
    output D3,
    output D4,
    output power,
    input rxd,
    output txd,
    output sd);

reg ready = 0;
reg [23:0] 	  divider;
reg [3:0] 	  rot;

reg [23:0]    powerdiv;

reg rxd_s, rxd_tmp;
reg rxd_old, rxd_rise;

/* Synchronize rxd with local clock */
always @(posedge clk) begin
    rxd_tmp <= rxd;
    rxd_s <= rxd_tmp;
end

/* detect rising edge of rxd */
always @(posedge clk) begin
    if(rxd_old == 1'b0 && rxd_s == 1)
        begin
            rxd_rise <= 1'b1;
        end
    else
        begin
           rxd_rise <= 1'b0;
        end
    rxd_old <= rxd_s;
end

always @(posedge clk) begin
    if (ready)
        begin
        if (divider ==  12000000 && rxd_rise == 1'b1) 
            begin
                divider <= 0;
                rot <= {rot[2:0], rot[3]};
            end
        else if (divider != 12000000) 
            divider <= divider + 1;
        end
    else 
        begin
            ready <= 1;
            rot <= 4'b1000;
            divider <= 0;
        end
end

assign D1 = rot[0];
assign D2 = rot[1];
assign D3 = rot[2];
assign D4 = rot[3];
assign power = 1;

assign sd = 0; /* active IR */
assign txd = 0; /* txd not used */

endmodule // top
