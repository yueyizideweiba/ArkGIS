<?xml version="1.0" encoding="UTF-8"?>
<StyledLayerDescriptor xmlns="http://www.opengis.net/sld" xsi:schemaLocation="http://www.opengis.net/sld http://schemas.opengis.net/sld/1.1.0/StyledLayerDescriptor.xsd" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xlink="http://www.w3.org/1999/xlink" xmlns:se="http://www.opengis.net/se" version="1.1.0" xmlns:ogc="http://www.opengis.net/ogc">
  <NamedLayer>
    <se:Name>wuhan</se:Name>
    <UserStyle>
      <se:Name>wuhan</se:Name>
      <se:FeatureTypeStyle>
        <se:Rule>
          <se:Name>Single symbol</se:Name>
          <se:PolygonSymbolizer>
            <se:Fill>
              <se:SvgParameter name="fill">#ffa5f9</se:SvgParameter>
              <se:SvgParameter name="fill-opacity">0.43</se:SvgParameter>
            </se:Fill>
          </se:PolygonSymbolizer>
          <se:PolygonSymbolizer>
            <se:Fill>
              <se:GraphicFill>
                <se:Graphic>
                  <se:Mark>
                    <se:WellKnownName>diamond</se:WellKnownName>
                    <se:Fill>
                      <se:SvgParameter name="fill">#ffa5f9</se:SvgParameter>
                      <se:SvgParameter name="fill-opacity">0.43</se:SvgParameter>
                    </se:Fill>
                    <se:Stroke/>
                  </se:Mark>
                  <se:Size>7</se:Size>
                  <se:Rotation/>
                  <se:VendorOption name="widthHeightFactor">1.33333</se:VendorOption>
                </se:Graphic>
              </se:GraphicFill>
            </se:Fill>
            <se:VendorOption name="graphic-margin">9 9</se:VendorOption>
          </se:PolygonSymbolizer>
          <!--SymbolLayerV2 RandomMarkerFill not implemented yet-->
        </se:Rule>
      </se:FeatureTypeStyle>
    </UserStyle>
  </NamedLayer>
</StyledLayerDescriptor>
